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

