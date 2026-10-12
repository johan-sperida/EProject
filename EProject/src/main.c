/*  
  Rui Santos & Sara Santos - Random Nerd Tutorials
  https://RandomNerdTutorials.com/esp-idf-esp32-web-server-access-point/
*/
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
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
#include "LittleFSLib.h"

// Replace with your Access Point (AP) SSID and Password
#define MY_ESP_WIFI_SSID      "ESP32_AP"
#define MY_ESP_WIFI_PASS      "password123"

static const char *TAG = "web_server_ap";

// Global AP netif handle
static esp_netif_t *ap_netif;

// HTML web page to serve 
static char html_page[3000];
static const char* HTML_PAGE_PATH = "/littlefs/UploadTest.txt";



/* Max length a file path can have on storage */
#define FILE_PATH_MAX (CONFIG_LITTLEFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[CONFIG_LITTLEFS_OBJ_NAME_LEN + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

void sleep_seconds(float seconds){
    vTaskDelay(pdMS_TO_TICKS(seconds*1000.0));
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

// HTTP GET handler for root "/"
//runs every time a client enters the ESP IP address in browser
//they access the root folder or smth
static esp_err_t root_get_handler(httpd_req_t *req)
{
    //says type of file to send
    httpd_resp_set_type(req, "text/html");

    //read web server code from LittleFS file
    html_page[0] = '\0';
    LittleFS_load_text_file(HTML_PAGE_PATH, html_page, sizeof(html_page));
    
    //sends said file (the one defined earlier)
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler to upload a file onto the server */
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/upload" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri + sizeof("/upload") - 1, sizeof(filepath));
    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than "
                            MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = ((struct file_server_data *)req->user_ctx)->scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))
        if (recieved <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf, 1, received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

// Start the HTTP server
static httpd_handle_t start_web_server(const char *base_path)
{
    //default config has port 80 as listening port
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    //starts the webserver
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
    }

    static struct file_server_data *server_data = NULL;

    if (server_data) {
        ESP_LOGE(TAG, "File server already started");
        return NULL;
    }

    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return NULL;
    }

    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);

    //configures and register GET handler
    httpd_uri_t uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = server_data
    };
    httpd_register_uri_handler(server, &uri_get);

    LittleFS_ensure_dir("/littlefs/data/");
    /* URI handler for uploading files to server */
    httpd_uri_t file_upload = {
        .uri       = "/upload/*",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = server_data    // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_upload);


    return server;
    
    
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

void app_main()
{
    ESP_LOGI(TAG, "Starting... Free heap: %d bytes", esp_get_free_heap_size());


    // Mount LittleFS
    ESP_LOGI(TAG, "Booting LittleFS...");
    esp_err_t ret = LittleFS_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed, stopping demo.");
        return;
    }

    // Initialize NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    ret = nvs_flash_init();
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

    const char* basepath = "/littlefs/data";
    httpd_handle_t server = start_web_server(basepath);
    if (server) {
        ESP_LOGI(TAG, "Web server initialized. Connect to the Access Point (AP) now!");
    }

}