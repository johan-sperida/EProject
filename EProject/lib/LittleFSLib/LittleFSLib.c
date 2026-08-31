\
/*
 * ESP32-S3 LittleFS Demo (ESP-IDF)
 *
 * Demonstrates how to mount LittleFS, write/append/read files, list directory
 * contents, and query filesystem usage.
 */

#include "LittleFSLib.h"

static const char *TAG = "LittleFSLib";


esp_err_t LittleFS_mount(void) 
{
    // Configuration for LittleFS
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Register and mount LittleFS
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_littlefs_register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0;
    size_t used = 0;

    // Query LittleFS usage
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS mounted at %s", conf.base_path);
        ESP_LOGI(TAG, "Partition: total=%u bytes, used=%u bytes", (unsigned)total, (unsigned)used);
    } else {
        ESP_LOGW(TAG, "Mounted, but failed to query info: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}


void LittleFS_unmount(void)
{
    // Unregister and unmount LittleFS
    esp_vfs_littlefs_unregister("littlefs");
    ESP_LOGI(TAG, "LittleFS unmounted");
}


void LittleFS_ensure_dir(const char *path)
{
    struct stat st;
    
    // Check if path exists
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return;
        }
        ESP_LOGW(TAG, "Path exists but is not a directory: %s", path);
        return;
    }

    // Create directory
    if (mkdir(path, 0775) == 0) {
        ESP_LOGI(TAG, "Created directory: %s", path);
        return;
    }

    ESP_LOGE(TAG, "mkdir failed for %s: errno=%d (%s)", path, errno, strerror(errno));
}


void LittleFS_write_text_file(const char *path, const char *text)
{
    // Open file for writing (overwrite)
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "fopen(w) failed for %s: errno=%d (%s)", path, errno, strerror(errno));
        return;
    }

    // Write text to file
    size_t n = fwrite(text, 1, strlen(text), f);
    
    // Close file
    fclose(f);

    ESP_LOGI(TAG, "Wrote %u bytes to %s", (unsigned)n, path);
}


void LittleFS_append_text_file(const char *path, const char *text)
{
    // Open file for appending
    FILE *f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "fopen(a) failed for %s: errno=%d (%s)", path, errno, strerror(errno));
        return;
    }

    // Append text to file
    size_t n = fwrite(text, 1, strlen(text), f);
    
    // Close file
    fclose(f);

    ESP_LOGI(TAG, "Appended %u bytes to %s", (unsigned)n, path);
}


void LittleFS_read_text_file(const char *path)
{
    // Open file for reading
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "fopen(r) failed for %s: errno=%d (%s)", path, errno, strerror(errno));
        return;
    }

    ESP_LOGI(TAG, "---- Begin file: %s ----", path);

    // Read and log lines
    char line[128];
    while (fgets(line, sizeof(line), f) != NULL) {
        // Lines already include '\n' typically
        ESP_LOGI(TAG, "%s", line);
    }

    // Close file
    fclose(f);
    ESP_LOGI(TAG, "---- End file ----");
}


void LittleFS_list_dir(const char *dirpath)
{
    // Open directory
    DIR *dir = opendir(dirpath);
    if (!dir) {
        ESP_LOGE(TAG, "opendir failed for %s: errno=%d (%s)", dirpath, errno, strerror(errno));
        return;
    }

    ESP_LOGI(TAG, "Directory listing for: %s", dirpath);

    // Read entries
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char fullpath[256];
        int written = snprintf(
            fullpath,
            sizeof(fullpath),
            "%s/%s",
            dirpath,
            ent->d_name
        );

        if (written < 0 || written >= (int)sizeof(fullpath)) {
            ESP_LOGW(TAG, "Path truncated, skipping: %s/%s", dirpath, ent->d_name);
            continue;
        }

        struct stat st;
        if (stat(fullpath, &st) == 0) {
            const char *type = S_ISDIR(st.st_mode) ? "DIR " : "FILE";
            ESP_LOGI(TAG, "  %s  %s  size=%u", type, ent->d_name, (unsigned)st.st_size);
        } else {
            ESP_LOGI(TAG, "  ?    %s", ent->d_name);
        }
    }

    // Close directory
    closedir(dir);
}


void LittleFS_show_fs_info(void)
{
    size_t total = 0;
    size_t used = 0;

    // Query LittleFS usage
    esp_err_t ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_littlefs_info failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "LittleFS usage: used=%u / total=%u bytes (free=%u bytes)",
             (unsigned)used, (unsigned)total, (unsigned)(total - used));
}
