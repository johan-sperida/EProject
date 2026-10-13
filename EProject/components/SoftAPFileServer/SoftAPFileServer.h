#ifndef SOFT_AP_FILE_SERVER_H
#define SOFT_AP_FILE_SERVER_H

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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Delay execution for a floating-point number of seconds.
 */
void sleep_seconds(float seconds);

/**
 * Build the full filesystem path from a request URI and return a pointer to the
 * file name portion after the base path.
 */
const char *get_path_from_uri(char *dest,
                             const char *base_path,
                             const char *uri,
                             size_t destsize);


/**
 * Handle an upload request and save the uploaded file under the configured base path.
 */
esp_err_t upload_post_handler(httpd_req_t *req);

/**
 * Set the HTTP response content type from the file extension.
 */
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

/**
 * Stream a file to the client in fixed-size chunks.
 */
esp_err_t send_file_in_chunks(httpd_req_t *req, const char *filepath, const char *filename);

/**
 * Send a JavaScript chunk that updates the page title with the LittleFS usage values.
 */
void send_fs_usage_chunk(httpd_req_t *req);

/**
 * Render the directory listing HTML, including per-file metadata and delete forms.
 */
esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);

/**
 * Serve a requested file or directory listing from LittleFS.
 * Includes root page
 */
esp_err_t download_get_handler(httpd_req_t *req);

/**
 * Delete a file from the server and redirect back to the root listing.
 */
esp_err_t delete_post_handler(httpd_req_t *req);

/**
 * Start the HTTP server using the given LittleFS base path.
 */
httpd_handle_t start_web_server(const char *base_path);

/**
 * Handle Wi-Fi AP events and log connection state changes.
 */
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/**
 * Entry point for the AP file-server application.
 */
void start_File_Server(const char* basepath);

#ifdef __cplusplus
}
#endif

#endif /* SOFT_AP_FILE_SERVER_H */
