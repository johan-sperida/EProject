#include <LittleFSLib.h>
#include "esp_err.h"


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