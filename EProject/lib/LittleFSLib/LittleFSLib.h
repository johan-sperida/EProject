#ifndef LITTLEFS_LIB_H
#define LITTLEFS_LIB_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mount LittleFS filesystem 
 * 
 * @return esp_err_t ESP_OK on success, otherwise failure code 
 */
esp_err_t LittleFS_mount(void);

/**
 * @brief Unmount LittleFS filesystem 
 * 
 */
void LittleFS_unmount(void);

/**
 * @brief Ensure a directory exists, create if needed 
 * 
 * @param path Path to directory 
 */
void LittleFS_ensure_dir(const char *path);

/**
 * @brief Write text to a file (overwrites existing content) 
 * 
 * @param path Path to text file to write 
 * @param text Text to write 
 */
void LittleFS_write_text_file(const char *path, const char *text);

/**
 * @brief Append text to a file 
 * 
 * @param path Path to text file to append to 
 * @param text Text to append 
 */
void LittleFS_append_text_file(const char *path, const char *text);

/**
 * @brief Read and log contents of a text file 
 * 
 * @param path Path to text file to read 
 */
void LittleFS_read_text_file(const char *path);

/**
 * @brief List directory contents   
 * 
 * @param dirpath Path to directory to list 
 */
void LittleFS_list_dir(const char *dirpath);

/**
 * @brief Show filesystem information
 * 
 */
void LittleFS_show_fs_info(void);

#ifdef __cplusplus
}
#endif

#endif /* LITTLEFS_LIB_H */


/*
#include <LittleFSLib.h>

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting LittleFS demo...");

    // Mount LittleFS
    esp_err_t ret = LittleFS_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed, stopping demo.");
        return;
    }

    // Prepare directories and files
    const char *base_dir = "/littlefs";
    const char *cfg_dir  = "/littlefs/config";
    const char *log_dir  = "/littlefs/logs";

    // Ensure directories exist
    LittleFS_ensure_dir(cfg_dir);
    LittleFS_ensure_dir(log_dir);

    // File paths
    const char *cfg_path = "/littlefs/config/device.cfg";
    const char *log_path = "/littlefs/logs/boot.log";

    // Write initial config file
    LittleFS_write_text_file(cfg_path,
                    "device_id=ESP32S3\n"
                    "mode=demo\n"
                    "wifi_autostart=false\n");

    // Append to config file
    LittleFS_append_text_file(cfg_path, "log_enabled=true\n");

    // Write initial log file
    LittleFS_append_text_file(log_path, "boot=ok\n");

    // Read back files
    LittleFS_read_text_file(cfg_path);

    // List directories
    LittleFS_list_dir(base_dir);
    LittleFS_list_dir(cfg_dir);
    LittleFS_list_dir(log_dir);

    // Show filesystem usage
    LittleFS_show_fs_info();

    // Keep running to allow monitor viewing; periodically append to log
    for (int i = 0; i < 5; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "tick=%d\n", i);
        LittleFS_append_text_file(log_path, buf);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Final read of log file
    LittleFS_read_text_file(log_path);
    
    // Final filesystem usage
    LittleFS_show_fs_info();

    // Optional unmount (usually not required in embedded apps)
    LittleFS_unmount();

    ESP_LOGI(TAG, "Demo complete.");
}
*/