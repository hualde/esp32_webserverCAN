#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
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
extern const uint8_t coding_guide_html_start[] asm("_binary_coding_guide_html_start");
extern const uint8_t coding_guide_html_end[]   asm("_binary_coding_guide_html_end");
extern const uint8_t endstop_guide_html_start[] asm("_binary_endstop_guide_html_start");
extern const uint8_t endstop_guide_html_end[]   asm("_binary_endstop_guide_html_end");
extern const uint8_t logo_jpg_start[] asm("_binary_logo_jpg_start");
extern const uint8_t logo_jpg_end[]   asm("_binary_logo_jpg_end");

/* CAN Config */
#define TX_GPIO_NUM 18   // GPIO para transmisión CAN
#define RX_GPIO_NUM 19   // GPIO para recepción CAN

/* Global variables */
static char current_lang[3] = "es";
static bool last_rx_valid = false;
static char last_rx_line[128] = {0};
static uint8_t pending_xx = 0x00;
static bool pending_xx_valid = false;
static bool action2_routine_started = false;

/* Seguridad UDS: desbloqueo por seed/key */
#define SEED_REQUEST_ID 0x712
#define SEED_RESPONSE_ID 0x77C
#define SEED_ADDEND 0x4B31u

/* Tester Present continuo (UDS) para la calibracion de topes (action2):
 * trama 02 3E 80 ... enviada en CAN ID 0x700 cada 500 ms. La subfuncion 0x80
 * indica "sin respuesta". Debe mantenerse ininterrumpido durante todo el proceso
 * (salvo el KEY OFF del ciclo de encendido), o la ECU sale de sesion (>2 s). */
#define TESTER_PRESENT_ID 0x700
static volatile bool tester_present_active = false;

/* --- Ejecución asíncrona + estado en vivo de action2 (feedback de flancos) ---
 * Los pasos de action2 pueden tardar (acción humana). Para no bloquear el servidor
 * web y poder mostrar progreso en pantalla, se ejecutan en una tarea de fondo y la
 * web consulta /api/step_status periódicamente. */
static SemaphoreHandle_t a2_start_sem = NULL;
static char a2_req_action[16] = {0};
static volatile int  a2_req_step = -1;
static volatile bool a2_worker_busy = false;
static volatile bool a2_worker_done = false;
static volatile bool a2_worker_success = false;

static volatile bool    a2_flank_valid = false;   /* hay lectura de 181B */
static volatile uint8_t a2_flank_b0 = 0;
static volatile uint8_t a2_flank_b1 = 0;
static volatile uint8_t a2_flank_b2 = 0;
static volatile uint8_t a2_flank_b3 = 0;
static volatile uint8_t a2_flank_b4 = 0;
static volatile bool    a2_auth_valid = false;     /* hay lectura de 1816 */
static volatile uint8_t a2_auth_a0 = 0xFF;
static volatile bool    a2_torque_valid = false;   /* hay lectura de par (DID 1805) */
static volatile int16_t a2_torque_mnm = 0;         /* par actual en mNm (signo = sentido) */
static volatile bool    a2_stop_pos_ok = false;    /* tope alcanzado en sentido + (>= +7500 mNm) */
static volatile bool    a2_stop_neg_ok = false;    /* tope alcanzado en sentido - (<= -7500 mNm) */

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

/* Envia Flow Control (30 00 00 ...) en 0x712 ante un First Frame (0x1X) recibido. */
static void send_flow_control(void) {
    const uint8_t fc[8] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_can_frame(SEED_REQUEST_ID, fc);
}

/* Tarea de Tester Present: mientras tester_present_active sea true, envia
 * 02 3E 80 ... en 0x700 cada 500 ms. La ECU no responde (subfuncion 0x80). */
static void tester_present_task(void *arg) {
    (void)arg;
    const uint8_t tp[8] = {0x02, 0x3E, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (;;) {
        if (tester_present_active) {
            twai_message_t message;
            memset(&message, 0, sizeof(message));
            message.identifier = TESTER_PRESENT_ID;
            message.extd = 0;
            message.rtr = 0;
            message.data_length_code = 8;
            memcpy(message.data, tp, 8);
            twai_transmit(&message, pdMS_TO_TICKS(50));
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
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

static bool uds_wait_with_pending(uint8_t req_sid,
                                  bool (*match_positive)(const twai_message_t *msg, void *ctx),
                                  void *ctx,
                                  int timeout_ms,
                                  int pending_timeout_ms) {
    const int64_t start_us = esp_timer_get_time();
    int64_t deadline_us = start_us + (int64_t)timeout_ms * 1000;
    const int64_t pending_deadline_us = start_us + (int64_t)pending_timeout_ms * 1000;
    twai_message_t rx_msg;

    for (;;) {
        int64_t now_us = esp_timer_get_time();
        if (now_us >= deadline_us) return false;

        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) != ESP_OK) {
            continue;
        }
        if (rx_msg.identifier != SEED_RESPONSE_ID || rx_msg.data_length_code < 3) {
            continue;
        }

        // Negative response: 7F <req_sid> <nrc>
        if (rx_msg.data_length_code >= 4 &&
            rx_msg.data[0] == 0x03 && rx_msg.data[1] == 0x7F && rx_msg.data[2] == req_sid) {
            uint8_t nrc = rx_msg.data[3];
            if (nrc == 0x78) {
                if (now_us >= pending_deadline_us) return false;
                continue;
            }
            return false;
        }

        if (match_positive && match_positive(&rx_msg, ctx)) return true;
    }
}

static bool match_7e_tester_present(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 3 &&
            m->data[0] == 0x02 && m->data[1] == 0x7E && m->data[2] == 0x00);
}

static bool match_50_1003(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 3 &&
            m->data[0] == 0x06 && m->data[1] == 0x50 && m->data[2] == 0x03);
}

typedef struct {
    uint32_t seed;
    bool got_seed;
} seed_ctx_t;

static bool match_67_seed(const twai_message_t *m, void *ctx) {
    seed_ctx_t *s = (seed_ctx_t *)ctx;
    if (!(m->data_length_code >= 7 && m->data[1] == 0x67 && m->data[2] == 0x03)) return false;
    s->seed = ((uint32_t)m->data[3] << 24) | ((uint32_t)m->data[4] << 16) | ((uint32_t)m->data[5] << 8) | (uint32_t)m->data[6];
    s->got_seed = true;
    return true;
}

static bool match_67_access_ok(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code == 8 &&
            m->data[0] == 0x02 && m->data[1] == 0x67 && m->data[2] == 0x04 &&
            m->data[3] == 0xAA && m->data[4] == 0xAA && m->data[5] == 0xAA && m->data[6] == 0xAA && m->data[7] == 0xAA);
}

static bool match_6e_f199(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 4 &&
            m->data[0] == 0x03 && m->data[1] == 0x6E && m->data[2] == 0xF1 && m->data[3] == 0x99);
}

static bool match_30_flow(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 3 &&
            m->data[0] == 0x30 && m->data[1] == 0x0F && m->data[2] == 0x03);
}

static bool match_6e_f198(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 4 &&
            m->data[0] == 0x03 && m->data[1] == 0x6E && m->data[2] == 0xF1 && m->data[3] == 0x98);
}

static bool match_6e_0600(const twai_message_t *m, void *ctx) {
    (void)ctx;
    return (m->data_length_code >= 4 &&
            m->data[0] == 0x03 && m->data[1] == 0x6E && m->data[2] == 0x06 && m->data[3] == 0x00);
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

/** Respuesta positiva UDS a ClearDiagnosticInformation (0x14): 0x54 (ISO-TP SF 01 54 … u otras variantes). */
static bool rx_is_clear_dtc_positive(const twai_message_t *m) {
    if (!m || m->identifier != SEED_RESPONSE_ID || m->data_length_code < 1) {
        return false;
    }
    const uint8_t *d = m->data;
    uint8_t dlc = m->data_length_code;

    if (dlc >= 2 && (d[0] & 0xF0) == 0x00) {
        unsigned sf_len = (unsigned)(d[0] & 0x0F);
        if (sf_len >= 1 && d[1] == 0x54) {
            return true;
        }
    }
    if (d[0] == 0x54) {
        return true;
    }
    if (dlc >= 2 && d[0] == 0x02 && d[1] == 0x54) {
        return true;
    }
    return false;
}

/* CAN Transmission Logic (returns true if access OK on action1/step0) */
static bool transmit_can_step(const char *action_key, int step_idx) {
    bool access_ok = false;
    bool action2_step_ok = false;
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
                
                bool seed_request_tx_ok = false;
                /* Paso 2 acción 1: la trama 07 2E 06 00 XX 1F 00 00 se envía más abajo con el byte XX del query.
                 * action2 maneja todas sus tramas de forma explícita en sus propios bloques. */
                bool skip_auto_send =
                    (strcmp(action_key, "action1") == 0 && (step_idx == 0 || step_idx == 1 || step_idx == 2 || step_idx == 3 || step_idx == 4)) ||
                    (strcmp(action_key, "action2") == 0);

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

                if (strcmp(action_key, "action1") == 0 &&
                    step_idx == 0) {
                    // Acción 1 paso 1: SecurityAccess. Reglas críticas:
                    // - NO enviar 04 14 aquí
                    // - TX -> esperar RX antes del siguiente TX
                    drain_rx_queue();
                    last_rx_valid = false;

                    const uint8_t tester_present[8] = {0x02,0x3E,0x00,0xFF,0xFF,0xFF,0xFF,0xFF};
                    const uint8_t diag_session[8]   = {0x02,0x10,0x03,0xFF,0xFF,0xFF,0xFF,0xFF};
                    const uint8_t seed_req[8]       = {0x02,0x27,0x03,0xFF,0xFF,0xFF,0xFF,0xFF};

                    if (!send_can_frame(SEED_REQUEST_ID, tester_present)) goto step0_done;
                    if (!uds_wait_with_pending(0x3E, match_7e_tester_present, NULL, 2000, 5000)) goto step0_done;

                    if (!send_can_frame(SEED_REQUEST_ID, diag_session)) goto step0_done;
                    if (!uds_wait_with_pending(0x10, match_50_1003, NULL, 2000, 5000)) goto step0_done;

                    if (!send_can_frame(SEED_REQUEST_ID, seed_req)) goto step0_done;
                    seed_ctx_t sctx = {.seed = 0, .got_seed = false};
                    if (!uds_wait_with_pending(0x27, match_67_seed, &sctx, 2000, 5000) || !sctx.got_seed) goto step0_done;

                    uint32_t key = sctx.seed + SEED_ADDEND;
                    twai_message_t key_msg;
                    memset(&key_msg, 0, sizeof(key_msg));
                    key_msg.identifier = SEED_REQUEST_ID;
                    key_msg.extd = 0;
                    key_msg.rtr = 0;
                    key_msg.data_length_code = 8;
                    key_msg.data[0] = 0x06;
                    key_msg.data[1] = 0x27;
                    key_msg.data[2] = 0x04;
                    key_msg.data[3] = (uint8_t)((key >> 24) & 0xFF);
                    key_msg.data[4] = (uint8_t)((key >> 16) & 0xFF);
                    key_msg.data[5] = (uint8_t)((key >> 8) & 0xFF);
                    key_msg.data[6] = (uint8_t)(key & 0xFF);
                    key_msg.data[7] = 0xFF;
                    if (twai_transmit(&key_msg, pdMS_TO_TICKS(100)) != ESP_OK) goto step0_done;
                    if (!uds_wait_with_pending(0x27, match_67_access_ok, NULL, 2000, 5000)) goto step0_done;

                    // Acción 1: ejecutar Paso 2 inmediatamente tras 67 04 (sin intervención humana)
                    if (strcmp(action_key, "action1") == 0) {
                        if (!pending_xx_valid) goto step0_done;

                        const uint8_t f199_req[8] = {0x06, 0x2E, 0xF1, 0x99, 0x26, 0x03, 0x06, 0xFF};
                        if (!send_can_frame(SEED_REQUEST_ID, f199_req)) goto step0_done;
                        if (!uds_wait_with_pending(0x2E, match_6e_f199, NULL, 2000, 5000)) goto step0_done;

                        const uint8_t f198_ff[8] = {0x10, 0x09, 0x2E, 0xF1, 0x98, 0x0A, 0x2C, 0x2F};
                        if (!send_can_frame(SEED_REQUEST_ID, f198_ff)) goto step0_done;
                        if (!uds_wait_with_pending(0x2E, match_30_flow, NULL, 2000, 5000)) goto step0_done;

                        const uint8_t f198_cf[8] = {0x21, 0xCF, 0x86, 0x9F, 0xFF, 0xFF, 0xFF, 0xFF};
                        if (!send_can_frame(SEED_REQUEST_ID, f198_cf)) goto step0_done;
                        if (!uds_wait_with_pending(0x2E, match_6e_f198, NULL, 2000, 5000)) goto step0_done;

                        const uint8_t write_coding[8] = {0x07, 0x2E, 0x06, 0x00, pending_xx, 0x1F, 0x00, 0x00};
                        if (!send_can_frame(SEED_REQUEST_ID, write_coding)) goto step0_done;
                        if (!uds_wait_with_pending(0x2E, match_6e_0600, NULL, 2000, 5000)) goto step0_done;
                    }

                    access_ok = true;
step0_done:
                    ;
                }

                // Paso 2 de Acción 1 ahora se ejecuta AUTOMÁTICO dentro del Paso 1 (step_idx==0).

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
                    ESP_LOGI(TAG, "[action1] Paso 4: reescritura fingerprint y fecha post-reset");
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

                /* ============================================================
                 * ACTION2 — Calibración de topes de dirección EPS (UDS sobre CAN)
                 * Tester Present 02 3E 80 ... en 0x700 cada 500 ms (tarea de fondo)
                 * activo desde el paso 1 hasta el final (salvo el KEY OFF del ciclo).
                 * ============================================================ */

                /* --- Paso 1-3: sesión extendida + Security Access (seed/key) --- */
                if (strcmp(action_key, "action2") == 0 && step_idx == 0) {
                    ESP_LOGI(TAG, "[action2] Paso 1-3: sesion extendida + security access");
                    tester_present_active = true;   /* arranca Tester Present 0x700 */
                    vTaskDelay(pdMS_TO_TICKS(50));
                    drain_rx_queue();
                    last_rx_valid = false;

                    const uint8_t diag_session[8] = {0x02,0x10,0x03,0x00,0x00,0x00,0x00,0x00};
                    const uint8_t seed_req[8]     = {0x02,0x27,0x03,0x00,0x00,0x00,0x00,0x00};

                    /* Paso 1: 10 03 -> 50 03 */
                    if (!send_can_frame(SEED_REQUEST_ID, diag_session)) goto a2_step0_done;
                    if (!uds_wait_with_pending(0x10, match_50_1003, NULL, 2000, 5000)) goto a2_step0_done;

                    /* Paso 2: 27 03 -> 67 03 [SEED] */
                    if (!send_can_frame(SEED_REQUEST_ID, seed_req)) goto a2_step0_done;
                    seed_ctx_t sctx = {.seed = 0, .got_seed = false};
                    if (!uds_wait_with_pending(0x27, match_67_seed, &sctx, 2000, 5000) || !sctx.got_seed) goto a2_step0_done;

                    /* Paso 3: KEY = SEED + 0x4B31 (mantiene los 2 bytes altos) -> 67 04 */
                    uint32_t key = sctx.seed + SEED_ADDEND;
                    ESP_LOGI(TAG, "[action2] Seed 0x%08lX -> Key 0x%08lX",
                             (unsigned long)sctx.seed, (unsigned long)key);
                    twai_message_t key_msg;
                    memset(&key_msg, 0, sizeof(key_msg));
                    key_msg.identifier = SEED_REQUEST_ID;
                    key_msg.extd = 0;
                    key_msg.rtr = 0;
                    key_msg.data_length_code = 8;
                    key_msg.data[0] = 0x06;
                    key_msg.data[1] = 0x27;
                    key_msg.data[2] = 0x04;
                    key_msg.data[3] = (uint8_t)((key >> 24) & 0xFF);
                    key_msg.data[4] = (uint8_t)((key >> 16) & 0xFF);
                    key_msg.data[5] = (uint8_t)((key >> 8) & 0xFF);
                    key_msg.data[6] = (uint8_t)(key & 0xFF);
                    key_msg.data[7] = 0xFF;
                    if (twai_transmit(&key_msg, pdMS_TO_TICKS(100)) != ESP_OK) goto a2_step0_done;
                    if (!uds_wait_with_pending(0x27, match_67_access_ok, NULL, 2000, 5000)) goto a2_step0_done;

                    access_ok = true;
a2_step0_done:
                    ;
                }

                /* --- Paso 4: reconocimiento de flancos (DID 181B) hasta 01 01 01 01 01 --- */
                if (strcmp(action_key, "action2") == 0 && step_idx == 1) {
                    ESP_LOGI(TAG, "[action2] Paso 4: reconocimiento de flancos (DID 181B)");
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;

                    bool flanks_ok = false;
                    const int64_t deadline_us = esp_timer_get_time() + (int64_t)120 * 1000000;

                    while (esp_timer_get_time() < deadline_us && !flanks_ok) {
                        const uint8_t read_181b[8] = {0x03,0x22,0x18,0x1B,0x00,0x00,0x00,0x00};
                        if (!send_can_frame(SEED_REQUEST_ID, read_181b)) {
                            vTaskDelay(pdMS_TO_TICKS(450));
                            continue;
                        }

                        /* La respuesta 62 18 1B lleva 5 bytes de flancos repartidos en
                         * First Frame (10 08 62 18 1B B0 B1 B2) y Consecutive Frame
                         * (21 B3 B4). Hace falta que LOS 5 lleguen a 01: en trazas reales
                         * una calibración con solo los 3 primeros a 01 fue rechazada por
                         * la EPS (1816 != 00 y DTC 003F0A persistente). */
                        uint8_t b[5] = {0xFF,0xFF,0xFF,0xFF,0xFF};
                        bool got_ff = false;
                        for (int i = 0; i < 20 && !got_ff; i++) {
                            if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                rx_msg.data[0] == 0x10 && rx_msg.data[2] == 0x62 &&
                                rx_msg.data[3] == 0x18 && rx_msg.data[4] == 0x1B) {
                                b[0] = rx_msg.data[5];
                                b[1] = rx_msg.data[6];
                                b[2] = rx_msg.data[7];
                                got_ff = true;
                            }
                        }

                        if (got_ff) {
                            send_flow_control();
                            bool got_cf = false;
                            for (int i = 0; i < 20 && !got_cf; i++) {
                                if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                                if (rx_msg.identifier == SEED_RESPONSE_ID && rx_msg.data[0] == 0x21) {
                                    b[3] = rx_msg.data[1];
                                    b[4] = rx_msg.data[2];
                                    got_cf = true;
                                }
                            }
                            ESP_LOGI(TAG, "[action2] 181B flancos B0..B4: %02X %02X %02X %02X %02X",
                                     b[0], b[1], b[2], b[3], b[4]);
                            /* Publica el estado en vivo para el feedback de pantalla */
                            a2_flank_b0 = b[0];
                            a2_flank_b1 = b[1];
                            a2_flank_b2 = b[2];
                            a2_flank_b3 = b[3];
                            a2_flank_b4 = b[4];
                            a2_flank_valid = true;
                            if (got_cf &&
                                b[0] == 0x01 && b[1] == 0x01 && b[2] == 0x01 &&
                                b[3] == 0x01 && b[4] == 0x01) {
                                snprintf(last_rx_line, sizeof(last_rx_line),
                                         "%08lX,false,Rx,0,8,62,18,1B,%02X,%02X,%02X,%02X,",
                                         (unsigned long)SEED_RESPONSE_ID, b[0], b[1], b[2], b[3]);
                                last_rx_valid = true;
                                flanks_ok = true;
                            }
                        }

                        if (!flanks_ok) vTaskDelay(pdMS_TO_TICKS(450));
                    }

                    action2_step_ok = flanks_ok;
                }

                /* --- Paso 6: iniciar rutina 31 01 04 16 + leer resultado 31 03 hasta RR==01 --- */
                if (strcmp(action_key, "action2") == 0 && step_idx == 2) {
                    ESP_LOGI(TAG, "[action2] Paso 6: iniciar rutina de calibracion (0416)");
                    drain_rx_queue();
                    last_rx_valid = false;
                    action2_routine_started = false;
                    twai_message_t rx_msg;

                    const uint8_t routine_start[8] = {0x04,0x31,0x01,0x04,0x16,0x00,0x00,0x00};
                    send_can_frame(SEED_REQUEST_ID, routine_start);

                    bool started = false;
                    for (int i = 0; i < 50; i++) {
                        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) != ESP_OK) continue;
                        if (rx_msg.identifier != SEED_RESPONSE_ID) continue;
                        if (rx_msg.data[1] == 0x71 && rx_msg.data[2] == 0x01 &&
                            rx_msg.data[3] == 0x04 && rx_msg.data[4] == 0x16) {
                            started = true;
                            break;
                        }
                        if (rx_msg.data[1] == 0x7F && rx_msg.data[2] == 0x31) {
                            if (rx_msg.data[3] == 0x78) continue;   /* responsePending */
                            ESP_LOGW(TAG, "[action2] StartRoutine NRC=0x%02X", rx_msg.data[3]);
                            break;
                        }
                    }
                    action2_routine_started = started;

                    /* Confirmar rutina en marcha: 31 03 hasta RR==01 (aunque el start
                     * devuelva NRC por estar ya activa, p. ej. en un reintento). */
                    bool captured = false;
                    {
                        const int64_t deadline_us = esp_timer_get_time() + (int64_t)15 * 1000000;
                        while (esp_timer_get_time() < deadline_us && !captured) {
                            const uint8_t routine_res[8] = {0x04,0x31,0x03,0x04,0x16,0x00,0x00,0x00};
                            send_can_frame(SEED_REQUEST_ID, routine_res);
                            for (int i = 0; i < 10; i++) {
                                if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                                if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                    rx_msg.data[1] == 0x71 && rx_msg.data[2] == 0x03 &&
                                    rx_msg.data[3] == 0x04 && rx_msg.data[4] == 0x16) {
                                    uint8_t rr = rx_msg.data[5];
                                    ESP_LOGI(TAG, "[action2] 31 03 RR=0x%02X (espera 0x01)", rr);
                                    if (rr == 0x01) {
                                        snprintf(last_rx_line, sizeof(last_rx_line),
                                                 "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                                 (unsigned long)rx_msg.identifier,
                                                 rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                                 rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                                        last_rx_valid = true;
                                        captured = true;
                                    }
                                    break;
                                }
                            }
                            if (!captured) vTaskDelay(pdMS_TO_TICKS(400));
                        }
                    }

                    if (captured) {
                        /* Acción humana: apretar contra cada tope con más de 7,5 Nm.
                         * Se monitoriza el par del sensor de torsión (DID 1805, mNm con
                         * signo) y se publica en vivo para la pantalla. El paso termina
                         * cuando se alcanzan +7500 y -7500 mNm (ambos topes). */
                        bool both_stops = false;
                        const int64_t tq_deadline_us = esp_timer_get_time() + (int64_t)180 * 1000000;
                        while (esp_timer_get_time() < tq_deadline_us && !both_stops) {
                            const uint8_t read_1805[8] = {0x03,0x22,0x18,0x05,0x00,0x00,0x00,0x00};
                            if (send_can_frame(SEED_REQUEST_ID, read_1805)) {
                                for (int i = 0; i < 10; i++) {
                                    if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                                    if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                        rx_msg.data[0] == 0x06 && rx_msg.data[1] == 0x62 &&
                                        rx_msg.data[2] == 0x18 && rx_msg.data[3] == 0x05) {
                                        int16_t tq = (int16_t)(((uint16_t)rx_msg.data[4] << 8) | rx_msg.data[5]);
                                        a2_torque_mnm = tq;
                                        a2_torque_valid = true;
                                        if (tq >= 7500)  a2_stop_pos_ok = true;
                                        if (tq <= -7500) a2_stop_neg_ok = true;
                                        ESP_LOGI(TAG, "[action2] 1805 par=%d mNm (+ok=%d -ok=%d)",
                                                 (int)tq, (int)a2_stop_pos_ok, (int)a2_stop_neg_ok);
                                        break;
                                    }
                                }
                            }
                            both_stops = a2_stop_pos_ok && a2_stop_neg_ok;
                            if (!both_stops) vTaskDelay(pdMS_TO_TICKS(300));
                        }
                        action2_step_ok = both_stops;
                    }
                }

                /* --- Pasos 8-9: barrido + validar (31 02 04 16) -> RR==02 -> confirmar 1816 A0==00 ---
                 * En las capturas reales el DID 1816 mantiene A0=0xFF durante toda la fase entre
                 * StartRoutine y StopRoutine; A0 solo pasa a 0x00 DESPUÉS del StopRoutine. Por eso
                 * la confirmación de "Autorizado" se hace aquí, tras validar la rutina. */
                if (strcmp(action_key, "action2") == 0 && step_idx == 3) {
                    ESP_LOGI(TAG, "[action2] Paso 9: validar adaptacion (StopRoutine 0416) + confirmar 1816");
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;

                    const uint8_t routine_stop[8] = {0x04,0x31,0x02,0x04,0x16,0x00,0x00,0x00};
                    send_can_frame(SEED_REQUEST_ID, routine_stop);
                    for (int i = 0; i < 50; i++) {
                        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) != ESP_OK) continue;
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data[1] == 0x71 && rx_msg.data[2] == 0x02 &&
                            rx_msg.data[3] == 0x04 && rx_msg.data[4] == 0x16) {
                            break;
                        }
                        if (rx_msg.identifier == SEED_RESPONSE_ID &&
                            rx_msg.data[1] == 0x7F && rx_msg.data[2] == 0x31 &&
                            rx_msg.data[3] != 0x78) {
                            ESP_LOGW(TAG, "[action2] StopRoutine NRC=0x%02X", rx_msg.data[3]);
                            break;
                        }
                    }

                    /* Leer resultado final: 31 03 hasta RR==02 (timeout 30 s) */
                    bool validated = false;
                    const int64_t deadline_us = esp_timer_get_time() + (int64_t)30 * 1000000;
                    while (esp_timer_get_time() < deadline_us && !validated) {
                        const uint8_t routine_res[8] = {0x04,0x31,0x03,0x04,0x16,0x00,0x00,0x00};
                        send_can_frame(SEED_REQUEST_ID, routine_res);
                        for (int i = 0; i < 10; i++) {
                            if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                rx_msg.data[1] == 0x71 && rx_msg.data[2] == 0x03 &&
                                rx_msg.data[3] == 0x04 && rx_msg.data[4] == 0x16) {
                                uint8_t rr = rx_msg.data[5];
                                ESP_LOGI(TAG, "[action2] 31 03 final RR=0x%02X (espera 0x02)", rr);
                                if (rr == 0x02) {
                                    snprintf(last_rx_line, sizeof(last_rx_line),
                                             "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                             (unsigned long)rx_msg.identifier,
                                             rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                             rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                                    last_rx_valid = true;
                                    validated = true;
                                }
                                break;
                            }
                        }
                        if (!validated) vTaskDelay(pdMS_TO_TICKS(400));
                    }

                    if (validated) {
                        /* Confirmación: tras el stop, el DID 1816 debe pasar a A0=0x00 (Autorizado).
                         * En trazas reales tarda ~10 s, por eso el margen es de 20 s. */
                        bool authorized = false;
                        const int64_t auth_deadline_us = esp_timer_get_time() + (int64_t)20 * 1000000;
                        while (esp_timer_get_time() < auth_deadline_us && !authorized) {
                            const uint8_t read_1816[8] = {0x03,0x22,0x18,0x16,0x00,0x00,0x00,0x00};
                            send_can_frame(SEED_REQUEST_ID, read_1816);
                            for (int i = 0; i < 10; i++) {
                                if (twai_receive(&rx_msg, pdMS_TO_TICKS(100)) != ESP_OK) continue;
                                if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                    rx_msg.data[0] == 0x07 && rx_msg.data[1] == 0x62 &&
                                    rx_msg.data[2] == 0x18 && rx_msg.data[3] == 0x16) {
                                    uint8_t a0 = rx_msg.data[4];
                                    ESP_LOGI(TAG, "[action2] 1816 A0=0x%02X (00=Autorizado)", a0);
                                    a2_auth_a0 = a0;
                                    a2_auth_valid = true;
                                    if (a0 == 0x00) authorized = true;
                                    break;
                                }
                            }
                            if (!authorized) vTaskDelay(pdMS_TO_TICKS(450));
                        }

                        action2_step_ok = true;
                        if (!authorized) {
                            ESP_LOGW(TAG, "[action2] RR=02 OK pero 1816 no confirmo Autorizado a tiempo");
                        }
                        /* El paso 10 (KEY OFF) corta la comunicación: detener Tester Present */
                        tester_present_active = false;
                        ESP_LOGI(TAG, "[action2] Calibracion validada. Tester Present detenido para KEY OFF");
                    }
                }

                /* --- Paso 10-12: tras KEY OFF/ON, reabrir sesión + borrar DTC + verificar --- */
                if (strcmp(action_key, "action2") == 0 && step_idx == 4) {
                    ESP_LOGI(TAG, "[action2] Paso 10-12: reabrir sesion + borrado DTC + verificacion");
                    tester_present_active = true;   /* reanudar Tester Present tras KEY ON */
                    vTaskDelay(pdMS_TO_TICKS(50));
                    drain_rx_queue();
                    last_rx_valid = false;
                    twai_message_t rx_msg;

                    /* Reabrir sesión extendida (10 03) */
                    const uint8_t diag_session[8] = {0x02,0x10,0x03,0x00,0x00,0x00,0x00,0x00};
                    send_can_frame(SEED_REQUEST_ID, diag_session);
                    uds_wait_with_pending(0x10, match_50_1003, NULL, 2000, 5000);

                    /* Paso 11: borrado DTC (04 14 FF FF FF), tolerando 7F 14 78 (pending) */
                    const uint8_t clear_dtc[8] = {0x04,0x14,0xFF,0xFF,0xFF,0x00,0x00,0x00};
                    send_can_frame(SEED_REQUEST_ID, clear_dtc);

                    bool got_dtc_ok = false;
                    const int64_t dtc_deadline_us = esp_timer_get_time() + (int64_t)8 * 1000000;
                    while (esp_timer_get_time() < dtc_deadline_us && !got_dtc_ok) {
                        if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) != ESP_OK) continue;
                        if (rx_msg.identifier != SEED_RESPONSE_ID) continue;
                        if (rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 && rx_msg.data[1] == 0x7F &&
                            rx_msg.data[2] == 0x14 && rx_msg.data[3] == 0x78) {
                            continue;   /* responsePending: esperar la respuesta real */
                        }
                        if (rx_msg.data_length_code >= 4 &&
                            rx_msg.data[0] == 0x03 && rx_msg.data[1] == 0x7F &&
                            rx_msg.data[2] == 0x14) {
                            ESP_LOGW(TAG, "[action2] Clear DTC NRC=0x%02X", rx_msg.data[3]);
                            break;
                        }
                        if (rx_is_clear_dtc_positive(&rx_msg)) {
                            got_dtc_ok = true;
                        }
                    }

                    if (got_dtc_ok) {
                        ESP_LOGI(TAG, "[action2] DTCs borrados OK");

                        /* Paso 12: verificación final (03 19 02 FF) — servicio 0x59, opcional */
                        const uint8_t read_dtc[8] = {0x03,0x19,0x02,0xFF,0x00,0x00,0x00,0x00};
                        send_can_frame(SEED_REQUEST_ID, read_dtc);
                        for (int i = 0; i < 25; i++) {
                            if (twai_receive(&rx_msg, pdMS_TO_TICKS(200)) != ESP_OK) continue;
                            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                                (rx_msg.data[1] == 0x59 || rx_msg.data[2] == 0x59)) {
                                snprintf(last_rx_line, sizeof(last_rx_line),
                                         "%08lX,false,Rx,0,8,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,",
                                         (unsigned long)rx_msg.identifier,
                                         rx_msg.data[0], rx_msg.data[1], rx_msg.data[2], rx_msg.data[3],
                                         rx_msg.data[4], rx_msg.data[5], rx_msg.data[6], rx_msg.data[7]);
                                last_rx_valid = true;
                                break;
                            }
                        }

                        action2_step_ok = true;
                    }

                    /* Proceso terminado: detener Tester Present */
                    tester_present_active = false;
                }
            }
        }
    }
    cJSON_Delete(root);
    if (strcmp(action_key, "action1") == 0 && step_idx >= 1 && step_idx <= 4) {
        return step_action1_ok;
    }
    if (strcmp(action_key, "action2") == 0 && step_idx >= 1 && step_idx <= 4) {
        return action2_step_ok;
    }
    return access_ok;
}

/* Worker: ejecuta un paso de action2 en segundo plano para no bloquear el servidor web */
static void action2_worker_task(void *arg) {
    (void)arg;
    for (;;) {
        xSemaphoreTake(a2_start_sem, portMAX_DELAY);
        a2_worker_done = false;
        a2_worker_success = false;
        bool r = transmit_can_step(a2_req_action, (int)a2_req_step);
        a2_worker_success = r;
        a2_worker_done = true;
        a2_worker_busy = false;
    }
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

static esp_err_t coding_guide_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)coding_guide_html_start,
                    coding_guide_html_end - coding_guide_html_start);
    return ESP_OK;
}

static esp_err_t endstop_guide_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)endstop_guide_html_start,
                    endstop_guide_html_end - endstop_guide_html_start);
    return ESP_OK;
}

static esp_err_t logo_jpg_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)logo_jpg_start, logo_jpg_end - logo_jpg_start);
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

            /* action2: ejecución asíncrona (worker) + feedback por /api/step_status */
            if (strcmp(action, "action2") == 0) {
                char aresp[64];
                if (a2_worker_busy) {
                    snprintf(aresp, sizeof(aresp), "{\"ok\":true,\"started\":false,\"busy\":true}");
                } else {
                    strncpy(a2_req_action, action, sizeof(a2_req_action) - 1);
                    a2_req_action[sizeof(a2_req_action) - 1] = '\0';
                    a2_req_step = step_idx;
                    a2_flank_valid = false;
                    a2_auth_valid = false;
                    a2_torque_valid = false;
                    a2_stop_pos_ok = false;
                    a2_stop_neg_ok = false;
                    a2_worker_done = false;
                    a2_worker_success = false;
                    a2_worker_busy = true;
                    xSemaphoreGive(a2_start_sem);
                    snprintf(aresp, sizeof(aresp), "{\"ok\":true,\"started\":true}");
                }
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, aresp, HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            }

            /* action1 (y resto): ejecución síncrona */
            bool result = transmit_can_step(action, step_idx);
            if (strcmp(action, "action1") == 0 && step_idx >= 1) {
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

// Estado en vivo del paso action2 en curso (feedback de flancos / autorización)
static esp_err_t step_status_get_handler(httpd_req_t *req) {
    char resp[352];
    snprintf(resp, sizeof(resp),
             "{\"busy\":%s,\"done\":%s,\"success\":%s,"
             "\"flank\":{\"valid\":%s,\"b0\":%u,\"b1\":%u,\"b2\":%u,\"b3\":%u,\"b4\":%u},"
             "\"torque\":{\"valid\":%s,\"mnm\":%d,\"pos\":%s,\"neg\":%s},"
             "\"auth\":{\"valid\":%s,\"a0\":%u}}",
             a2_worker_busy ? "true" : "false",
             a2_worker_done ? "true" : "false",
             a2_worker_success ? "true" : "false",
             a2_flank_valid ? "true" : "false",
             (unsigned)a2_flank_b0, (unsigned)a2_flank_b1, (unsigned)a2_flank_b2,
             (unsigned)a2_flank_b3, (unsigned)a2_flank_b4,
             a2_torque_valid ? "true" : "false", (int)a2_torque_mnm,
             a2_stop_pos_ok ? "true" : "false", a2_stop_neg_ok ? "true" : "false",
             a2_auth_valid ? "true" : "false", (unsigned)a2_auth_a0);
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
static const httpd_uri_t coding_guide_uri = { .uri = "/guia-codificacion.html", .method = HTTP_GET, .handler = coding_guide_get_handler };
static const httpd_uri_t endstop_guide_uri = { .uri = "/guia-topes.html", .method = HTTP_GET, .handler = endstop_guide_get_handler };
static const httpd_uri_t logo_jpg_uri = { .uri = "/logo.jpg", .method = HTTP_GET, .handler = logo_jpg_get_handler };
static const httpd_uri_t execute_step_uri = { .uri = "/api/execute_step", .method = HTTP_POST, .handler = execute_step_post_handler };
static const httpd_uri_t set_lang_uri = { .uri = "/api/set_lang", .method = HTTP_POST, .handler = set_lang_post_handler };
static const httpd_uri_t last_rx_uri = { .uri = "/api/last_rx", .method = HTTP_GET, .handler = last_rx_get_handler };
static const httpd_uri_t step_status_uri = { .uri = "/api/step_status", .method = HTTP_GET, .handler = step_status_get_handler };

/* Start Web Server */
static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16;   /* por defecto son 8; tenemos 11 rutas registradas */

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &style_uri);
        httpd_register_uri_handler(server, &script_uri);
        httpd_register_uri_handler(server, &step_json_uri);
        httpd_register_uri_handler(server, &coding_guide_uri);
        httpd_register_uri_handler(server, &endstop_guide_uri);
        httpd_register_uri_handler(server, &logo_jpg_uri);
        httpd_register_uri_handler(server, &execute_step_uri);
        httpd_register_uri_handler(server, &set_lang_uri);
        httpd_register_uri_handler(server, &last_rx_uri);
        httpd_register_uri_handler(server, &step_status_uri);
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

    // Tarea de Tester Present (inactiva hasta que action2 la habilite)
    xTaskCreate(tester_present_task, "tester_present", 3072, NULL, 5, NULL);

    // Worker asíncrono de action2 (feedback de flancos sin bloquear el servidor)
    a2_start_sem = xSemaphoreCreateBinary();
    xTaskCreate(action2_worker_task, "action2_worker", 8192, NULL, 5, NULL);

    // Start Server
    start_webserver();
}
