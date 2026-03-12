#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

/* Seguridad UDS: desbloqueo por seed/key */
#define SEED_REQUEST_ID 0x712
#define SEED_RESPONSE_ID 0x77C
#define SEED_ADDEND 0x4B31u
#define SEED_RESPONSE_TIMEOUT_MS 500
#define KEY_RESPONSE_TIMEOUT_MS 500

static bool wait_for_seed_response(uint32_t *seed_out) {
    if (!seed_out) return false;
    twai_message_t rx_msg;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(SEED_RESPONSE_TIMEOUT_MS);

    while ((xTaskGetTickCount() - start) < timeout) {
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(20)) == ESP_OK) {
            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                rx_msg.data_length_code >= 7 &&
                rx_msg.data[1] == 0x67 &&
                rx_msg.data[2] == 0x03) {
                *seed_out = ((uint32_t)rx_msg.data[3] << 24) |
                            ((uint32_t)rx_msg.data[4] << 16) |
                            ((uint32_t)rx_msg.data[5] << 8)  |
                            ((uint32_t)rx_msg.data[6]);
                return true;
            }
        }
    }
    return false;
}

static bool wait_for_key_response(void) {
    twai_message_t rx_msg;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(KEY_RESPONSE_TIMEOUT_MS);

    while ((xTaskGetTickCount() - start) < timeout) {
        if (twai_receive(&rx_msg, pdMS_TO_TICKS(20)) == ESP_OK) {
            if (rx_msg.identifier == SEED_RESPONSE_ID &&
                rx_msg.data_length_code >= 3 &&
                rx_msg.data[1] == 0x67 &&
                rx_msg.data[2] == 0x04) {
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
    return false;
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

/* CAN Transmission Logic */
static void transmit_can_step(const char *action_key, int step_idx) {
    const char *json_data = (const char *)can_frames_json_start;
    cJSON *root = cJSON_ParseWithLength(json_data, can_frames_json_end - can_frames_json_start);
    if (!root) {
        ESP_LOGE(TAG, "Error parsing CAN JSON");
        return;
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

                    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
                        ESP_LOGI(TAG, "  -> CAN Transmit: ID 0x%03lX", id);
                    } else {
                        ESP_LOGE(TAG, "  !! Failed to transmit CAN frame 0x%03lX", id);
                    }

                    if (strcmp(action_key, "action1") == 0 && step_idx == 0 &&
                        id == SEED_REQUEST_ID && dlc >= 3 &&
                        message.data[0] == 0x02 && message.data[1] == 0x27 &&
                        message.data[2] == 0x03) {
                        last_rx_valid = false;
                        uint32_t seed = 0;
                        if (wait_for_seed_response(&seed)) {
                            uint32_t key = seed + SEED_ADDEND;
                            ESP_LOGI(TAG, "  <- Seed 0x%08lX, Key 0x%08lX", seed, key);
                            send_key_response(key);
                            if (!wait_for_key_response()) {
                                ESP_LOGE(TAG, "  !! Timeout esperando respuesta 0x67 0x04");
                            }
                        } else {
                            ESP_LOGE(TAG, "  !! Timeout esperando seed 0x%03X", SEED_RESPONSE_ID);
                        }
                    }

                    if (delay > 0) vTaskDelay(pdMS_TO_TICKS(delay));
                }
            }
        }
    }
    cJSON_Delete(root);
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
    if (ret == ESP_OK) {
        char action[32];
        char step_str[10];
        if (httpd_query_key_value(buf, "action", action, sizeof(action)) == ESP_OK &&
            httpd_query_key_value(buf, "step", step_str, sizeof(step_str)) == ESP_OK) {
            
            int step_idx = atoi(step_str);
            transmit_can_step(action, step_idx);
        }
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
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
            .ssid = "ESP32_Control_AP",
            .ssid_len = strlen("ESP32_Control_AP"),
            .channel = 1,
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP iniciado. SSID: %s Clave: %s", wifi_config.ap.ssid, wifi_config.ap.password);
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
