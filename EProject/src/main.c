#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "SoftAPFileServer.h"
#include "LittleFSLib.h"
static const char* TAG = "app main";

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

    const char* basepath = "/littlefs/data";
    start_File_Server(basepath);
}