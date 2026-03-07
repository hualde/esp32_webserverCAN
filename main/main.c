#include <stdio.h>
#include <string.h>
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
#define TX_GPIO_NUM 18   // GPIO para transmisión CAN
#define RX_GPIO_NUM 19   // GPIO para recepción CAN

/* Global variables */
static char current_lang[3] = "es";

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
                    uint32_t id = cJSON_GetObjectItem(frame_obj, "id")->valueint;
                    cJSON *data_arr = cJSON_GetObjectItem(frame_obj, "data");
                    uint8_t dlc = cJSON_GetObjectItem(frame_obj, "dlc")->valueint;
                    int delay = cJSON_GetObjectItem(frame_obj, "delay_ms")->valueint;

                    twai_message_t message;
                    message.identifier = id;
                    message.extd = 0;
                    message.data_length_code = dlc;
                    for (int j = 0; j < dlc && j < 8; j++) {
                        message.data[j] = cJSON_GetArrayItem(data_arr, j)->valueint;
                    }

                    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
                        ESP_LOGI(TAG, "  -> CAN Transmit: ID 0x%03lX", id);
                    } else {
                        ESP_LOGE(TAG, "  !! Failed to transmit CAN frame 0x%03lX", id);
                    }

                    if (delay > 0) vTaskDelay(pdMS_TO_TICKS(delay));
                }
            }
        }
    }
    cJSON_Delete(root);
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
