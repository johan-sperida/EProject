#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h" // needed to save wifi state
#include "mdns.h" //add manually as a component idf components
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"



void app_main(void) {
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    IP4_ADDR(&info.ip, 192, 168, 10, 1);
    IP4_ADDR(&info.gw, 192, 168, 10, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
}