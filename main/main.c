#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "driver/twai.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static const char *TAG = "ESP32_WEB_APP";

/* Embed files declarations */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");
extern const uint8_t script_js_start[]  asm("_binary_script_js_start");
extern const uint8_t script_js_end[]    asm("_binary_script_js_end");
extern const uint8_t can_frames_json_start[] asm("_binary_can_frames_json_start");
extern const uint8_t can_frames_json_end[]   asm("_binary_can_frames_json_end");

/* CAN Config */
#define TX_GPIO_NUM 17   // GPIO para transmisión CAN
#define RX_GPIO_NUM 16   // GPIO para recepción CAN

/* Global variables */
static char current_lang[3] = "es";
static bool last_rx_valid = false;
static char last_rx_line[128] = {0};
static uint8_t pending_xx = 0x00;
static bool pending_xx_valid = false;

/* Seguridad UDS: desbloqueo por seed/key */
#define SEED_REQUEST_ID 0x712
#define SEED_RESPONSE_ID 0x77C
#define SEED_ADDEND 0x4B31u

static void send_key_response(uint32_t key);

static bool send_can_frame(uint32_t id, const uint8_t data[8]) {
    twai_message_t message;
    memset(&message, 0, sizeof(message));
    message.identifier = id;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 8;
    memcpy(message.data, data, 8);

    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        ESP_LOGI(TAG, "  -> CAN Transmit: ID 0x%03" PRIX32, id);
        return true;
    }
    ESP_LOGE(TAG, "  !! Failed to transmit CAN frame 0x%03" PRIX32, id);
    return false;
}

static bool wait_for_access_ok(void) {
    twai_message_t rx_msg;
    bool key_sent = false;

    for (;;) {
        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) {
            continue;
        }

        if (rx_msg.identifier == SEED_RESPONSE_ID &&
            rx_msg.data_length_code >= 7 &&
            rx_msg.data[1] == 0x67 &&
            rx_msg.data[2] == 0x03 &&
            !key_sent) {
            uint32_t seed = ((uint32_t)rx_msg.data[3] << 24) |
                            ((uint32_t)rx_msg.data[4] << 16) |
                            ((uint32_t)rx_msg.data[5] << 8)  |
                            ((uint32_t)rx_msg.data[6]);
            uint32_t key = seed + SEED_ADDEND;
            ESP_LOGI(TAG, "  <- Seed 0x%08lX, Key 0x%08lX", seed, key);
            send_key_response(key);
            key_sent = true;
            continue;
        }

        if (rx_msg.identifier == SEED_RESPONSE_ID &&
            rx_msg.data_length_code == 8 &&
            rx_msg.data[0] == 0x02 &&
            rx_msg.data[1] == 0x67 &&
            rx_msg.data[2] == 0x04 &&
            rx_msg.data[3] == 0xAA &&
            rx_msg.data[4] == 0xAA &&
            rx_msg.data[5] == 0xAA &&
            rx_msg.data[6] == 0xAA &&
            rx_msg.data[7] == 0xAA) {
            snprintf(last_rx_line, sizeof(last_rx_line),
                     "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                     (unsigned long)rx_msg.identifier,
                     rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                     rx_msg.data[4], rx_msg.data[5], rx_msg.data[6],
                     (rx_msg.data_length_code > 7 ? rx_msg.data[7] : 0x00));
            last_rx_valid = true;
            return true;
        }
    }
}


static void drain_rx_queue(void) {
    twai_message_t rx_msg;
    while (twai_receive(&rx_msg, 0) == ESP_OK) {
        // Descarta mensajes antiguos en cola RX
    }
}

static void send_key_response(uint32_t key) {
    twai_message_t message;
    memset(&message, 0, sizeof(message));
    message.identifier = SEED_REQUEST_ID;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 8;
    message.data[0] = 0x06;
    message.data[1] = 0x27;
    message.data[2] = 0x04;
    uint8_t k1 = (uint8_t)((key >> 24) & 0xFF);
    uint8_t k2 = (uint8_t)((key >> 16) & 0xFF);
    uint8_t k3 = (uint8_t)((key >> 8) & 0xFF);
    uint8_t k4 = (uint8_t)(key & 0xFF);
    message.data[3] = k1;
    message.data[4] = k2;
    message.data[5] = k3;
    message.data[6] = k4;
    message.data[7] = 0xFF;

    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        ESP_LOGI(TAG, "  -> CAN Transmit Key: ID 0x%03X", SEED_REQUEST_ID);
    } else {
        ESP_LOGE(TAG, "  !! Failed to transmit CAN key frame 0x%03X", SEED_REQUEST_ID);
    }
}

static bool send_seed_request_frame(void) {
    twai_message_t message;
    memset(&message, 0, sizeof(message));
    message.identifier = SEED_REQUEST_ID;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 8;
    message.data[0] = 0x02;
    message.data[1] = 0x27;
    message.data[2] = 0x03;
    message.data[3] = 0xFF;
    message.data[4] = 0xFF;
    message.data[5] = 0xFF;
    message.data[6] = 0xFF;
    message.data[7] = 0xFF;

    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        ESP_LOGI(TAG, "  -> CAN Transmit (fallback): ID 0x%03X", SEED_REQUEST_ID);
        return true;
    }

    ESP_LOGE(TAG, "  !! Failed to transmit seed request 0x%03X", SEED_REQUEST_ID);
    return false;
}

static bool parse_query_byte(const char *str, uint8_t *out) {
    if (!str || !out) return false;
    char *endptr = NULL;
    unsigned long val = strtoul(str, &endptr, 0);
    if (endptr == str) return false;
    if (val > 0xFF) val &= 0xFF;
    *out = (uint8_t)val;
    return true;
}

/* Parsea un valor que puede ser número (decimal) o string (hex: "0x064", "064", "64") */
static uint32_t parse_can_id_or_byte(cJSON *item) {
    if (!item) return 0;
    if (cJSON_IsNumber(item))
        return (uint32_t)(item->valueint & 0xFF);
    if (cJSON_IsString(item) && item->valuestring) {
        return (uint32_t)strtoul(item->valuestring, NULL, 16);
    }
    return 0;
}

/* Parsea ID CAN: número decimal o string en hex (ej. "0x064", "064") */
static uint32_t parse_can_id(cJSON *item) {
    if (!item) return 0;
    if (cJSON_IsNumber(item))
        return (uint32_t)(item->valueint & 0x7FF);
    if (cJSON_IsString(item) && item->valuestring)
        return (uint32_t)strtoul(item->valuestring, NULL, 16);
    return 0;
}

/* CAN Transmission Logic (returns true if access OK on action1/step0) */
static bool transmit_can_step(const char *action_key, int step_idx) {
    bool access_ok = false;
    bool step2_action2_ok = false;
    bool step3_action2_ok = false;
    bool step4_action2_ok = false;
    bool step_action1_ok = false;
    const char *json_data = (const char *)can_frames_json_start;
    cJSON *root = cJSON_ParseWithLength(json_data, can_frames_json_end - can_frames_json_start);
    if (!root) {
        ESP_LOGE(TAG, "Error parsing CAN JSON");
        return false;
    }

    cJSON *action_obj = cJSON_GetObjectItem(root, action_key);
    if (action_obj) {
        cJSON *steps_array = cJSON_GetObjectItem(action_obj, "steps");
        if (cJSON_IsArray(steps_array) && step_idx < cJSON_GetArraySize(steps_array)) {
            cJSON *step_obj = cJSON_GetArrayItem(steps_array, step_idx);
            cJSON *frames_array = cJSON_GetObjectItem(step_obj, "frames");
            
            if (cJSON_IsArray(frames_array)) {
                int frames_count = cJSON_GetArraySize(frames_array);
                ESP_LOGI(TAG, "Ejecutando %s - Paso %d (%d tramas)", action_key, step_idx + 1, frames_count);
                
                if (strcmp(action_key, "action2") == 0 && step_idx == 0) {
                    // Evita borrar respuestas válidas que lleguen justo tras el 27 03
                    drain_rx_queue();
                }

                bool seed_request_tx_ok = false;
                bool skip_auto_send =
                    (strcmp(action_key, "action1") == 0 && (step_idx == 2 || step_idx == 3 || step_idx == 4)) ||
                    (strcmp(action_key, "action2") == 0 && (step_idx == 1 || step_idx == 3));

                if (!skip_auto_send) {
                    for (int i = 0; i < frames_count; i++) {
                        cJSON *frame_obj = cJSON_GetArrayItem(frames_array, i);
                        cJSON *id_item = cJSON_GetObjectItem(frame_obj, "id");
                        uint32_t id = parse_can_id(id_item);
                        cJSON *data_arr = cJSON_GetObjectItem(frame_obj, "data");
                        uint8_t dlc = (uint8_t)(cJSON_GetObjectItem(frame_obj, "dlc")->valueint & 0x0F);
                        int delay = cJSON_GetObjectItem(frame_obj, "delay_ms")->valueint;

                        twai_message_t message;
                        memset(&message, 0, sizeof(message));
                        message.identifier = id;
                        message.extd = 0;
                        message.rtr = 0;   /* 0 = data frame (con payload); 1 = RTR (sin datos) */
                        message.data_length_code = dlc;
                        for (int j = 0; j < dlc && j < 8; j++) {
                            cJSON *item = cJSON_GetArrayItem(data_arr, j);
                            message.data[j] = (uint8_t)parse_can_id_or_byte(item);
                        }

                        if (strcmp(action_key, "action1") == 0 && step_idx == 1 &&
                            id == SEED_REQUEST_ID && dlc >= 5 && pending_xx_valid) {
                            message.data[4] = pending_xx;
                        }

                        bool tx_ok = (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK);
                        if (tx_ok) {
                            ESP_LOGI(TAG, "  -> CAN Transmit: ID 0x%03lX", id);
                        } else {
                            ESP_LOGE(TAG, "  !! Failed to transmit CAN frame 0x%03lX", id);
                        }

                        if ((strcmp(action_key, "action1") == 0 || strcmp(action_key, "action2") == 0) &&
                            step_idx == 0 && id == SEED_REQUEST_ID && dlc >= 3 &&
                            message.data[0] == 0x02 && message.data[1] == 0x27 &&
                            message.data[2] == 0x03) {
                            seed_request_tx_ok = tx_ok;
                        }

                        if (delay > 0) vTaskDelay(pdMS_TO_TICKS(delay));
                    }
                }

                if ((strcmp(action_key, "action1") == 0 || strcmp(action_key, "action2") == 0) &&
                    step_idx == 0) {
                    ESP_LOGI(TAG, "[%s] Paso 1: esperando acceso (67 04)", action_key);
                    if (!seed_request_tx_ok) {
                        seed_request_tx_ok = send_seed_request_frame();
                    }

                    if (seed_request_tx_ok) {
                        if (strcmp(action_key, "action1") == 0) {
                            drain_rx_queue();
                        }
                        last_rx_valid = false;
                        if (wait_for_access_ok()) {
                            access_ok = true;
                        }
                    }
                }

                if (strcmp(action_key, "action1") == 0 && step_idx == 1) {
                    ESP_LOGI(TAG, "[action1] Paso 2: esperando 6E 06 00");
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;
                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 &&
                            rx_msg.data[1] == 0x6E &&
                            rx_msg.data[2] == 0x06 &&
                            rx_msg.data[3] == 0x00) {
                            step_action1_ok = true;
                            break;
                        }
                    }
                }

                if (strcmp(action_key, "action1") == 0 && step_idx == 2) {
                    ESP_LOGI(TAG, "[action1] Paso 3: reset y sesion");
                    drain_rx_queue();
                    last_rx_valid = false;
                    const uint8_t reset_req[8] = {0x02,0x11,0x02,0xFF,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, reset_req);

                    bool got_reset_ok = false;
                    twai_message_t rx_msg;
                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 &&
                            rx_msg.data[1] == 0x7F &&
                            rx_msg.data[2] == 0x11 &&
                            rx_msg.data[3] == 0x78) {
                            continue;
                        }
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 3 &&
                            rx_msg.data[0] == 0x02 &&
                            rx_msg.data[1] == 0x51 &&
                            rx_msg.data[2] == 0x02) {
                            got_reset_ok = true;
                            break;
                        }
                    }

                    if (got_reset_ok) {
                        const uint8_t tester_present[8] = {0x02,0x3E,0x00,0xFF,0xFF,0xFF,0xFF,0xFF};
                        const uint8_t diag_session[8] = {0x02,0x10,0x03,0xFF,0xFF,0xFF,0xFF,0xFF};
                        send_can_frame(SEED_REQUEST_ID, tester_present);
                        send_can_frame(SEED_REQUEST_ID, diag_session);
                        step_action1_ok = true;
                    }
                }

                if (strcmp(action_key, "action1") == 0 && step_idx == 3) {
                    ESP_LOGI(TAG, "[action1] Paso 4: fingerprint y fecha");
                    drain_rx_queue();
                    last_rx_valid = false;
                    const uint8_t f198_ff[8] = {0x10,0x09,0x2E,0xF1,0x98,0x0A,0x2C,0x2F};
                    send_can_frame(SEED_REQUEST_ID, f198_ff);

                    twai_message_t rx_msg;
                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 3 &&
                            rx_msg.data[0] == 0x30 &&
                            rx_msg.data[1] == 0x0F &&
                            rx_msg.data[2] == 0x03) {
                            break;
                        }
                    }

                    const uint8_t f198_cf[8] = {0x21,0xCF,0x86,0x9F,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, f198_cf);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 &&
                            rx_msg.data[1] == 0x6E &&
                            rx_msg.data[2] == 0xF1 &&
                            rx_msg.data[3] == 0x98) {
                            break;
                        }
                    }

                    const uint8_t f199_req[8] = {0x06,0x2E,0xF1,0x99,0x26,0x03,0x06,0xFF};
                    send_can_frame(SEED_REQUEST_ID, f199_req);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 &&
                            rx_msg.data[1] == 0x6E &&
                            rx_msg.data[2] == 0xF1 &&
                            rx_msg.data[3] == 0x99) {
                            step_action1_ok = true;
                            break;
                        }
                    }
                }

                if (strcmp(action_key, "action1") == 0 && step_idx == 4) {
                    ESP_LOGI(TAG, "[action1] Paso 5: verificacion");
                    drain_rx_queue();
                    last_rx_valid = false;
                    const uint8_t read_req[8] = {0x03,0x22,0x06,0x00,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, read_req);

                    twai_message_t rx_msg;
                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x07 &&
                            rx_msg.data[1] == 0x62 &&
                            rx_msg.data[2] == 0x06 &&
                            rx_msg.data[3] == 0x00 &&
                            (!pending_xx_valid || rx_msg.data[4] == pending_xx) &&
                            rx_msg.data[5] == 0x1F &&
                            rx_msg.data[6] == 0x00 &&
                            rx_msg.data[7] == 0x00) {
                            snprintf(last_rx_line, sizeof(last_rx_line),
                                     "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                     (unsigned long)rx_msg.identifier,
                                     rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                     rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                            last_rx_valid = true;
                            step_action1_ok = true;
                            break;
                        }
                    }
                }

                if (strcmp(action_key, "action2") == 0 && step_idx == 1) {
                    ESP_LOGI(TAG, "[action2] Paso 2: esperando 62 18 1B FF/CF y 71 01/71 03");
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;

                    const uint8_t read_181b[8] = {0x03,0x22,0x18,0x1B,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, read_181b);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x10 &&
                            rx_msg.data[1] == 0x08 &&
                            rx_msg.data[2] == 0x62 &&
                            rx_msg.data[3] == 0x18 &&
                            rx_msg.data[4] == 0x1B &&
                            rx_msg.data[5] == 0x01 &&
                            rx_msg.data[6] == 0x01 &&
                            rx_msg.data[7] == 0x01) {
                            break;
                        }
                    }

                    const uint8_t fc_181b[8] = {0x30,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, fc_181b);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x21 &&
                            rx_msg.data[1] == 0x01 &&
                            rx_msg.data[2] == 0x01) {
                            break;
                        }
                    }

                    const uint8_t routine_01[8] = {0x04,0x31,0x01,0x04,0x16,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, routine_01);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x04 &&
                            rx_msg.data[1] == 0x71 &&
                            rx_msg.data[2] == 0x01 &&
                            rx_msg.data[3] == 0x04 &&
                            rx_msg.data[4] == 0x16) {
                            break;
                        }
                    }

                    const uint8_t routine_03[8] = {0x04,0x31,0x03,0x04,0x16,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, routine_03);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x07 &&
                            rx_msg.data[1] == 0x71 &&
                            rx_msg.data[2] == 0x03 &&
                            rx_msg.data[3] == 0x04 &&
                            rx_msg.data[4] == 0x16 &&
                            rx_msg.data[5] == 0x01) {
                            snprintf(last_rx_line, sizeof(last_rx_line),
                                     "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                     (unsigned long)rx_msg.identifier,
                                     rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                     rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                            last_rx_valid = true;
                            step2_action2_ok = true;
                            break;
                        }
                    }
                }

                if (strcmp(action_key, "action2") == 0 && step_idx == 2) {
                    ESP_LOGI(TAG, "[action2] Paso 3: polling 22 18 16 (400ms)");
                    twai_message_t rx_msg;
                    for (;;) {
                        twai_message_t poll_msg;
                        memset(&poll_msg, 0, sizeof(poll_msg));
                        poll_msg.identifier = SEED_REQUEST_ID;
                        poll_msg.extd = 0;
                        poll_msg.rtr = 0;
                        poll_msg.data_length_code = 8;
                        poll_msg.data[0] = 0x03;
                        poll_msg.data[1] = 0x22;
                        poll_msg.data[2] = 0x18;
                        poll_msg.data[3] = 0x16;
                        poll_msg.data[4] = 0xFF;
                        poll_msg.data[5] = 0xFF;
                        poll_msg.data[6] = 0xFF;
                        poll_msg.data[7] = 0xFF;

                        if (twai_transmit(&poll_msg, pdMS_TO_TICKS(100)) != ESP_OK) {
                            ESP_LOGE(TAG, "  !! Failed to transmit CAN frame 0x%03X (22 18 16)", SEED_REQUEST_ID);
                        }

                        TickType_t start = xTaskGetTickCount();
                        TickType_t window = pdMS_TO_TICKS(400);
                        while ((xTaskGetTickCount() - start) < window) {
                            if (twai_receive(&rx_msg, pdMS_TO_TICKS(50)) != ESP_OK) {
                                continue;
                            }
                            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                rx_msg.data_length_code == 8 &&
                                rx_msg.data[0] == 0x07 &&
                                rx_msg.data[1] == 0x62 &&
                                rx_msg.data[2] == 0x18 &&
                                rx_msg.data[3] == 0x16 &&
                                rx_msg.data[4] == 0x00 &&
                                rx_msg.data[5] == 0x01 &&
                                rx_msg.data[6] == 0x01 &&
                                rx_msg.data[7] == 0x01) {
                                snprintf(last_rx_line, sizeof(last_rx_line),
                                         "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                         (unsigned long)rx_msg.identifier,
                                         rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                         rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                                last_rx_valid = true;
                                continue;
                            }
                            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                rx_msg.data_length_code == 8 &&
                                rx_msg.data[0] == 0x07 &&
                                rx_msg.data[1] == 0x62 &&
                                rx_msg.data[2] == 0x18 &&
                                rx_msg.data[3] == 0x16 &&
                                rx_msg.data[4] == 0x00 &&
                                rx_msg.data[5] == 0x00 &&
                                rx_msg.data[6] == 0x01 &&
                                rx_msg.data[7] == 0x00) {
                                snprintf(last_rx_line, sizeof(last_rx_line),
                                         "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                         (unsigned long)rx_msg.identifier,
                                         rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                         rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                                last_rx_valid = true;
                                step3_action2_ok = true;
                                break;
                            }
                        }
                        if (step3_action2_ok) break;
                    }
                }

                if (strcmp(action_key, "action2") == 0 && step_idx == 3) {
                    ESP_LOGI(TAG, "[action2] Paso 4: esperando 62 19 23");
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;

                    const uint8_t routine_02[8] = {0x04,0x31,0x02,0x04,0x16,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, routine_02);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x04 &&
                            rx_msg.data[1] == 0x71 &&
                            rx_msg.data[2] == 0x02 &&
                            rx_msg.data[3] == 0x04 &&
                            rx_msg.data[4] == 0x16) {
                            break;
                        }
                    }

                    const uint8_t routine_03b[8] = {0x04,0x31,0x03,0x04,0x16,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, routine_03b);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x07 &&
                            rx_msg.data[1] == 0x71 &&
                            rx_msg.data[2] == 0x03 &&
                            rx_msg.data[3] == 0x04 &&
                            rx_msg.data[4] == 0x16 &&
                            rx_msg.data[5] == 0x02) {
                            break;
                        }
                    }

                    const uint8_t read_1923[8] = {0x03,0x22,0x19,0x23,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, read_1923);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x10 &&
                            rx_msg.data[1] == 0x09 &&
                            rx_msg.data[2] == 0x62 &&
                            rx_msg.data[3] == 0x19 &&
                            rx_msg.data[4] == 0x23 &&
                            rx_msg.data[5] == 0x01 &&
                            rx_msg.data[6] == 0x00 &&
                            rx_msg.data[7] == 0x28) {
                            break;
                        }
                    }

                    const uint8_t fc_1923[8] = {0x30,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, fc_1923);

                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code == 8 &&
                            rx_msg.data[0] == 0x21 &&
                            rx_msg.data[1] == 0x64 &&
                            rx_msg.data[2] == 0xD7 &&
                            rx_msg.data[3] == 0xE2) {
                            break;
                        }
                    }

                    const uint8_t clear_msg[8] = {0x04,0x14,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
                    send_can_frame(SEED_REQUEST_ID, clear_msg);

                    bool got_dtc_ok = false;
                    for (;;) {
                        if (twai_receive(&rx_msg, portMAX_DELAY) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 &&
                            rx_msg.data[1] == 0x7F &&
                            rx_msg.data[2] == 0x14 &&
                            rx_msg.data[3] == 0x78) {
                            continue;
                        }
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data_length_code >= 2 &&
                            rx_msg.data[0] == 0x01 &&
                            rx_msg.data[1] == 0x54) {
                            got_dtc_ok = true;
                            break;
                        }
                    }

                    if (got_dtc_ok) {
                        snprintf(last_rx_line, sizeof(last_rx_line),
                                 "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                 (unsigned long)rx_msg.identifier,
                                 rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                 (rx_msg.data_length_code > 4 ? rx_msg.data[4] : 0x00),
                                 (rx_msg.data_length_code > 5 ? rx_msg.data[5] : 0x00),
                                 (rx_msg.data_length_code > 6 ? rx_msg.data[6] : 0x00),
                                 (rx_msg.data_length_code > 7 ? rx_msg.data[7] : 0x00));
                        last_rx_valid = true;
                        step4_action2_ok = true;
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
    if (strcmp(action_key, "action1") == 0 && step_idx >= 1 && step_idx <= 4) {
        return step_action1_ok;
    }
    if (step_idx == 1 && strcmp(action_key, "action2") == 0) return step2_action2_ok;
    if (step_idx == 2 && strcmp(action_key, "action2") == 0) return step3_action2_ok;
    if (step_idx == 3 && strcmp(action_key, "action2") == 0) return step4_action2_ok;
    return access_ok;
}

static esp_err_t last_rx_get_handler(httpd_req_t *req) {
    char resp[192];
    if (last_rx_valid) {
        snprintf(resp, sizeof(resp), "{\"valid\":true,\"key\":\"%s\"}", last_rx_line);
    } else {
        snprintf(resp, sizeof(resp), "{\"valid\":false,\"key\":\"\"}");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handlers */

// Serve index.html
static esp_err_t index_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

// Serve style.css
static esp_err_t style_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

// Serve script.js
static esp_err_t script_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)script_js_start, script_js_end - script_js_start);
    return ESP_OK;
}

// Serve can_frames.json
static esp_err_t step_json_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)can_frames_json_start, can_frames_json_end - can_frames_json_start);
    return ESP_OK;
}

// Handle API Execute Step (Parameters: action, step)
static esp_err_t execute_step_post_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    bool access_ok = false;
    bool step_ok = false;
    if (ret == ESP_OK) {
        char action[32];
        char step_str[10];
        char xx_str[16] = {0};
        if (httpd_query_key_value(buf, "action", action, sizeof(action)) == ESP_OK &&
            httpd_query_key_value(buf, "step", step_str, sizeof(step_str)) == ESP_OK) {
            
            int step_idx = atoi(step_str);
            pending_xx_valid = false;
            if (httpd_query_key_value(buf, "xx", xx_str, sizeof(xx_str)) == ESP_OK) {
                uint8_t parsed = 0;
                if (parse_query_byte(xx_str, &parsed)) {
                    pending_xx = parsed;
                    pending_xx_valid = true;
                }
            }

            bool result = transmit_can_step(action, step_idx);
            if (strcmp(action, "action1") == 0 && step_idx >= 1) {
                step_ok = result;
            } else if (strcmp(action, "action2") == 0 && step_idx >= 1) {
                step_ok = result;
            } else {
                access_ok = result;
            }
        }
    }
    char resp[96];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"access_ok\":%s,\"step_ok\":%s}",
             access_ok ? "true" : "false", step_ok ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handle Language change API
static esp_err_t set_lang_post_handler(httpd_req_t *req) {
    char buf[32];
    int ret = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (ret == ESP_OK) {
        char lang[3];
        if (httpd_query_key_value(buf, "lang", lang, sizeof(lang)) == ESP_OK) {
            strncpy(current_lang, lang, 2);
            current_lang[2] = '\0';
            ESP_LOGI(TAG, "Idioma cambiado a: %s", current_lang);
        }
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI structures */
static const httpd_uri_t index_uri = { .uri = "/", .method = HTTP_GET, .handler = index_get_handler };
static const httpd_uri_t style_uri = { .uri = "/style.css", .method = HTTP_GET, .handler = style_get_handler };
static const httpd_uri_t script_uri = { .uri = "/script.js", .method = HTTP_GET, .handler = script_get_handler };
static const httpd_uri_t step_json_uri = { .uri = "/can_frames.json", .method = HTTP_GET, .handler = step_json_get_handler };
static const httpd_uri_t execute_step_uri = { .uri = "/api/execute_step", .method = HTTP_POST, .handler = execute_step_post_handler };
static const httpd_uri_t set_lang_uri = { .uri = "/api/set_lang", .method = HTTP_POST, .handler = set_lang_post_handler };
static const httpd_uri_t last_rx_uri = { .uri = "/api/last_rx", .method = HTTP_GET, .handler = last_rx_get_handler };

/* Start Web Server */
static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &style_uri);
        httpd_register_uri_handler(server, &script_uri);
        httpd_register_uri_handler(server, &step_json_uri);
        httpd_register_uri_handler(server, &execute_step_uri);
        httpd_register_uri_handler(server, &set_lang_uri);
        httpd_register_uri_handler(server, &last_rx_uri);
        ESP_LOGI(TAG, "Web server iniciado en puerto %d", config.server_port);
        return server;
    }
    ESP_LOGE(TAG, "Fallo al iniciar el servidor web");
    return NULL;
}

/* Wi-Fi Handler */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Estación conectada: "MACSTR", AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Estación desconectada: "MACSTR", AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/* Initialize Wi-Fi SoftAP */
void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Lizarte Configurator",
            .ssid_len = strlen("Lizarte Configurator"),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP iniciado. SSID: %s (abierto)", wifi_config.ap.ssid);
}

/* Initialize CAN (TWAI) Driver */
void can_init(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Standard 500kbps for Clio/Vehicle CAN
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI instalado");
    } else {
        ESP_LOGE(TAG, "Fallo al instalar driver TWAI");
        return;
    }

    if (twai_start() == ESP_OK) {
        ESP_LOGI(TAG, "Driver TWAI iniciado");
    } else {
        ESP_LOGE(TAG, "Fallo al iniciar driver TWAI");
    }
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Iniciando ESP32 Multilingual Web App...");

    // Init Wi-Fi
    wifi_init_softap();

    // Init CAN
    can_init();

    // Start Server
    start_webserver();
}
