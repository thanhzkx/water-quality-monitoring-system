#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

void wifi_init(void);

bool wifi_is_connected(void);

typedef void (*wifi_scan_callback_t)(
    const char *ssid,
    int rssi,
    bool first
);

void wifi_scan(wifi_scan_callback_t callback);

bool wifi_save_and_connect(
    const char *ssid,
    const char *password
);

bool wifi_delete_config(void);

#endif