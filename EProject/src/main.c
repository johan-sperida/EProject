/*  
  Rui Santos & Sara Santos - Random Nerd Tutorials
  https://RandomNerdTutorials.com/esp-idf-esp32-web-server-access-point/
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h" //events like wifi changes
#include "esp_log.h"

// stores key-value data in non-volatile storage (NVS) memory persistently 
#include "nvs_flash.h"

#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "sdkconfig.h"

// Replace with your Access Point (AP) SSID and Password
#define MY_ESP_WIFI_SSID      "ESP32_AP"
#define MY_ESP_WIFI_PASS      "password123"

static const char *TAG = "web_server_ap";

// Global AP netif handle
static esp_netif_t *ap_netif;

// HTML web page to serve 
// Save and read this in littlefs???? TODO
static const char *html_page; 

// R"raw(
// <!DOCTYPE html>
// <html>
// <head>
//     <title>ESP-IDF: ESP32 Web Server</title>
//     <meta name="viewport" content="width=device-width, initial-scale=1">
// </head>
// <body>
//     <h1>ESP-IDF: ESP32 Web Server</h1>
//     <p>Hello from ESP32!</p>
// </body>
// </html>
// )raw";

void sleep_seconds(float seconds){
    vTaskDelay(pdMS_TO_TICKS(seconds*1000.0));
}

// HTTP GET handler for root "/"
//runs every time a client enters the ESP IP address in browser
//they access the root folder or smth
static esp_err_t root_get_handler(httpd_req_t *req)
{
    //says type of file to send
    httpd_resp_set_type(req, "text/html");

    //sends said file (the one defined earlier)
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Start the HTTP server
static httpd_handle_t start_web_server(void)
{
    //default config has port 80 as listening port
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    //starts the webserver
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);

        //configures and registers 
        httpd_uri_t uri_get = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_get);
        return server;
    }
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}

// Wi-Fi and IP event handler
// handles any event for Wi-Fi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    //initialization (recieving ip)
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "Wi-Fi AP started! SSID: %s", MY_ESP_WIFI_SSID);

        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "AP IP: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "Web server ready → http://" IPSTR "/", IP2STR(&ip_info.ip));
        }
    
    //device connected
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Device connected: " MACSTR, MAC2STR(event->mac));
    
    //device disconnected
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Device disconnected: " MACSTR, MAC2STR(event->mac));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting... Free heap: %d bytes", esp_get_free_heap_size());

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP stack and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi STA interface
    ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler, NULL, NULL));

    // Configure Wi-Fi Access Point (AP) Configuration
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = MY_ESP_WIFI_SSID,
            .ssid_len = strlen(MY_ESP_WIFI_SSID),
            .channel = 1,
            .password = MY_ESP_WIFI_PASS,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    // Safety checks for correct SSID/Password to prevent reboots
    if (strlen(MY_ESP_WIFI_SSID) == 0 || strlen(MY_ESP_WIFI_SSID) > 32) {
        ESP_LOGE(TAG, "Invalid SSID length!");
        return;
    }
    if (strlen(MY_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGW(TAG, "No password → Open AP (anyone can connect)");
    } else if (strlen(MY_ESP_WIFI_PASS) < 8) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        wifi_config.ap.password[0] = '\0';

        //make a note of this!!!!
        ESP_LOGW(TAG, "Password too short (<8 chars) → Forced Open AP");

    } else {
        ESP_LOGI(TAG, "Using WPA2-PSK (password OK)");
    }

    // Start Wi-Fi interface in Access Point (AP) Mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Start web server in AP mode
    httpd_handle_t server = start_web_server();
    if (server) {
        ESP_LOGI(TAG, "Web server initialized. Connect to the Access Point (AP) now!");
    }

    while (1){
        sleep_seconds(0.5);
        ESP_LOGI(TAG, "loop continuing");
    }
}