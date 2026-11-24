#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_err.h"

#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI";

#define WIFI_CONNECTED_BIT BIT0

#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASSWORD_MAX_LEN   64

static EventGroupHandle_t wifi_event_group;

static esp_netif_t *sta_netif = NULL;
//static bool wifi_forgetting = false;
static bool wifi_connected = false;


/* =========================================================
 * NVS
 * ========================================================= */

static bool load_wifi_config(
    char *ssid,
    size_t ssid_size,
    char *password,
    size_t password_size
)
{
    nvs_handle_t nvs;

    esp_err_t err =
        nvs_open("wifi", NVS_READONLY, &nvs);

    if (err != ESP_OK)
    {
        return false;
    }

    size_t ssid_len = ssid_size;
    size_t password_len = password_size;

    err = nvs_get_str(
        nvs,
        "ssid",
        ssid,
        &ssid_len
    );

    if (err != ESP_OK)
    {
        nvs_close(nvs);
        return false;
    }

    err = nvs_get_str(
        nvs,
        "password",
        password,
        &password_len
    );

    nvs_close(nvs);

    if (err != ESP_OK)
    {
        return false;
    }

    return true;
}


/* =========================================================
 * SAVE WIFI
 * ========================================================= */

static bool save_wifi_config(
    const char *ssid,
    const char *password
)
{
    nvs_handle_t nvs;

    esp_err_t err =
        nvs_open(
            "wifi",
            NVS_READWRITE,
            &nvs
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NVS open failed: %s",
            esp_err_to_name(err)
        );

        return false;
    }

    err = nvs_set_str(
        nvs,
        "ssid",
        ssid
    );

    if (err != ESP_OK)
    {
        nvs_close(nvs);
        return false;
    }

    err = nvs_set_str(
        nvs,
        "password",
        password
    );

    if (err != ESP_OK)
    {
        nvs_close(nvs);
        return false;
    }

    err = nvs_commit(nvs);

    nvs_close(nvs);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "NVS commit failed"
        );

        return false;
    }

    ESP_LOGI(
        TAG,
        "WiFi configuration saved"
    );

    return true;
}


/* =========================================================
 * WIFI EVENT
 * ========================================================= */

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            ESP_LOGI(
                TAG,
                "STA started"
            );

            esp_wifi_connect();
        }

        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
    wifi_connected = false;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

    ESP_LOGW(TAG, "WiFi disconnected");

    // if (wifi_forgetting)
    // {
    //     ESP_LOGI(TAG, "WiFi forget mode - no reconnect");
    //     return;
    // }

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Trying to reconnect...");
    esp_wifi_connect();
}
    }

    else if (
        event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP
    )
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Got IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        wifi_connected = true;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}


/* =========================================================
 * WIFI INIT
 * ========================================================= */
static bool wifi_load_config(
    char *ssid,
    size_t ssid_size,
    char *password,
    size_t password_size
)
{
    nvs_handle_t handle;

    if (nvs_open("wifi", NVS_READONLY, &handle) != ESP_OK)
        return false;

    esp_err_t err1 = nvs_get_str(
        handle, "ssid", ssid, &ssid_size
    );

    esp_err_t err2 = nvs_get_str(
        handle, "password", password, &password_size
    );

    nvs_close(handle);

    return (err1 == ESP_OK && err2 == ESP_OK);
}
void wifi_init(void)
{
    wifi_event_group =
        xEventGroupCreate();

    sta_netif =
        esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    char ssid[WIFI_SSID_MAX_LEN + 1] = {0};

    char password[
        WIFI_PASSWORD_MAX_LEN + 1
    ] = {0};

    if (
        load_wifi_config(
            ssid,
            sizeof(ssid),
            password,
            sizeof(password)
        )
    )
    {
        ESP_LOGI(
            TAG,
            "Saved WiFi found: %s",
            ssid
        );

        wifi_config_t wifi_config = {0};

        strncpy(
            (char *)wifi_config.sta.ssid,
            ssid,
            sizeof(wifi_config.sta.ssid)
        );

        strncpy(
            (char *)wifi_config.sta.password,
            password,
            sizeof(wifi_config.sta.password)
        );

        ESP_ERROR_CHECK(
            esp_wifi_set_config(
                WIFI_IF_STA,
                &wifi_config
            )
        );
    }
   else
{
    ESP_LOGW(
        TAG,
        "No saved WiFi configuration - using default WiFi"
    );

    strcpy(ssid, "DESKTOP-N0V7TT89954");
    strcpy(password, "12345678");

    wifi_config_t wifi_config = {0};

    strcpy(
        (char *)wifi_config.sta.ssid,
        ssid
    );

    strcpy(
        (char *)wifi_config.sta.password,
        password
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );
}

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );
    EventBits_t bits = xEventGroupWaitBits(
    wifi_event_group,
    WIFI_CONNECTED_BIT,
    pdFALSE,
    pdFALSE,
    pdMS_TO_TICKS(15000)
);

if (bits & WIFI_CONNECTED_BIT)
{
    ESP_LOGI(TAG, "WiFi connected - ready for MQTT");
}
else
{
    ESP_LOGW(TAG, "WiFi connection timeout");
}
}


/* =========================================================
 * WIFI STATUS
 * ========================================================= */

bool wifi_is_connected(void)
{
    return wifi_connected;
}


/* =========================================================
 * WIFI SCAN
 * ========================================================= */

void wifi_scan(
    wifi_scan_callback_t callback
)
{
    if (!callback)
        return;

    /*
     * Nếu STA đang connect thì chờ nó ổn định.
     */
    if (!wifi_connected)
    {
        ESP_LOGW(
            TAG,
            "WiFi not connected, scan may fail"
        );
    }

    ESP_LOGI(
        TAG,
        "Starting WiFi scan..."
    );

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    esp_err_t err =
        esp_wifi_scan_start(
            &scan_config,
            true
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "WiFi scan failed: %s",
            esp_err_to_name(err)
        );

        return;
    }

    uint16_t count = 0;

    err =
        esp_wifi_scan_get_ap_num(
            &count
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Get AP number failed: %s",
            esp_err_to_name(err)
        );

        return;
    }

    if (count > 30)
        count = 30;

    wifi_ap_record_t *records =
        calloc(
            30,
            sizeof(wifi_ap_record_t)
        );

    if (!records)
    {
        ESP_LOGE(
            TAG,
            "Cannot allocate scan buffer"
        );

        return;
    }

    err =
        esp_wifi_scan_get_ap_records(
            &count,
            records
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Get AP records failed: %s",
            esp_err_to_name(err)
        );

        free(records);

        return;
    }

    bool first = true;

    for (int i = 0; i < count; i++)
    {
        if (
            strlen(
                (char *)records[i].ssid
            ) == 0
        )
        {
            continue;
        }

        callback(
            (char *)records[i].ssid,
            records[i].rssi,
            first
        );

        first = false;
    }

    free(records);

    ESP_LOGI(
        TAG,
        "WiFi scan completed: %d networks",
        count
    );
}

bool wifi_save_and_connect(
    const char *ssid,
    const char *password
)
{
    if (ssid == NULL || password == NULL)
    {
        ESP_LOGE(TAG, "Invalid WiFi credentials");
        return false;
    }

    ESP_LOGI(TAG, "Saving WiFi configuration: %s", ssid);

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        "wifi",
        NVS_READWRITE,
        &nvs_handle
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "NVS open failed: %s",
                 esp_err_to_name(err));

        return false;
    }

    err = nvs_set_str(
        nvs_handle,
        "ssid",
        ssid
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Save SSID failed");
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_set_str(
        nvs_handle,
        "password",
        password
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Save password failed");
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_commit(nvs_handle);

    nvs_close(nvs_handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "NVS commit failed: %s",
                 esp_err_to_name(err));

        return false;
    }

    ESP_LOGI(TAG, "WiFi configuration saved");

    /* Cấu hình WiFi mới */

wifi_config_t wifi_config = {0};

strncpy(
    (char *)wifi_config.sta.ssid,
    ssid,
    sizeof(wifi_config.sta.ssid) - 1
);

strncpy(
    (char *)wifi_config.sta.password,
    password,
    sizeof(wifi_config.sta.password) - 1
);

wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

ESP_ERROR_CHECK(
    esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    )
);

ESP_LOGI(
    TAG,
    "Switching to WiFi: %s",
    ssid
);

esp_wifi_disconnect();

vTaskDelay(pdMS_TO_TICKS(500));

esp_wifi_connect();

return true;
}
bool wifi_delete_config(void)
{
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return false;
    }

    nvs_erase_key(nvs_handle, "ssid");
    nvs_erase_key(nvs_handle, "password");

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit WiFi deletion: %s",
                 esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "WiFi configuration deleted");

    // WiFi mặc định
    const char *default_ssid = "DESKTOP-N0V7TT89954";
    const char *default_password = "12345678";

    wifi_config_t wifi_config = {0};

    strncpy((char *)wifi_config.sta.ssid,
            default_ssid,
            sizeof(wifi_config.sta.ssid) - 1);

    strncpy((char *)wifi_config.sta.password,
            default_password,
            sizeof(wifi_config.sta.password) - 1);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config)
    );

    ESP_LOGI(TAG, "Switching to default WiFi: %s", default_ssid);

    esp_wifi_disconnect();

    vTaskDelay(pdMS_TO_TICKS(500));

    esp_wifi_connect();

    return true;
}