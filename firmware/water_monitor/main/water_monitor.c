#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_adc/adc_oneshot.h"
#include "nvs_flash.h"

//WiFi + Network
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include <stdbool.h>
#include "freertos/event_groups.h"
//MQTT
#include "mqtt.h"
#include "wifi_manager.h"
// Sensor
#include "ds18b20.h"
#include "ph_sensor.h"
#include "turbidity.h"
#include "adc_utils.h"
#include "sensor_status.h"

//AI
#include "ai_anomaly.h"
#include "mqtt_client.h"
static const char *TAG = "MAIN";
static char device_id[20];

// //WIFI CONFIG
// #define WIFI_SSID "DESKTOP-N0V7TT89954"
// #define WIFI_PASS "****"

//TURBIDITY SENSOR
#define TUR_CHANNEL ADC_CHANNEL_X       
#define TURBIDITY_CHANNEL ADC_CHANNEL_1


// //WIFI
// void wifi_init(void)
// {
//     esp_netif_create_default_wifi_sta();
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = WIFI_SSID,
//             .password = WIFI_PASS,
//         },
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());
//     ESP_ERROR_CHECK(esp_wifi_connect());
//     ESP_LOGI(TAG, "WiFi connecting...");
// }

static void device_id_init(void)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(
        esp_read_mac(mac, ESP_MAC_WIFI_STA)
    );
    snprintf(device_id,
             sizeof(device_id),
             "%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);
    ESP_LOGI(TAG, "Device ID: %s", device_id);
}
static char wifi_scan_json[4096];
static int wifi_scan_json_pos = 0;

static void wifi_scan_callback(
    const char *ssid,
    int rssi,
    bool first
)
{
    char item[256];

    snprintf(
        item,
        sizeof(item),
        "%s{\"ssid\":\"%s\",\"rssi\":%d}",
        first ? "" : ",",
        ssid,
        rssi
    );

    int len = strlen(item);

    if (
        wifi_scan_json_pos + len <
        sizeof(wifi_scan_json) - 2
    )
    {
        memcpy(
            &wifi_scan_json[wifi_scan_json_pos],
            item,
            len
        );

        wifi_scan_json_pos += len;

        wifi_scan_json[wifi_scan_json_pos] = '\0';
    }
}
static void send_wifi_scan_result(void)
{
    wifi_scan_json_pos = 0;

    strcpy(
        wifi_scan_json,
        "{\"device_id\":\""
    );

    strcat(
        wifi_scan_json,
        device_id
    );

    strcat(
        wifi_scan_json,
        "\",\"networks\":["
    );

    wifi_scan_json_pos =
        strlen(wifi_scan_json);

    wifi_scan(
        wifi_scan_callback
    );

    strcat(
        wifi_scan_json,
        "]}");

    mqtt_publish(
        "factory/device/wifi/scan/result",
        wifi_scan_json
    );

    ESP_LOGI(
        "WIFI",
        "WiFi scan result sent to MQTT"
    );

    ESP_LOGI(
        "WIFI",
        "%s",
        wifi_scan_json
    );
}
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // WiFi
    device_id_init();
    wifi_init();
    
    // // Wait for WiFi connection
    // vTaskDelay(pdMS_TO_TICKS(5000));
    adc_init();
    // MQTT
    mqtt_start();
    while (!mqtt_is_connected())
{
    vTaskDelay(pdMS_TO_TICKS(500));
}

ESP_LOGI(TAG, "MQTT ready - scanning WiFi");

send_wifi_scan_result();
//     vTaskDelay(pdMS_TO_TICKS(5000));

// if (mqtt_is_connected())
// {
//     ESP_LOGI(TAG, "MQTT ready - scanning WiFi");

//     send_wifi_scan_result();
// }
// else
// {
//     ESP_LOGW(TAG, "MQTT not ready - cannot send WiFi scan");
// }
    // while (!mqtt_is_connected())
    // {
    //     ESP_LOGI(TAG, "Waiting for MQTT connection...");
    //     vTaskDelay(pdMS_TO_TICKS(500));
    // }
    ESP_LOGI(TAG, "MQTT connected!");
    srand(esp_random());
    // AI initialization
    ESP_LOGI(TAG, "BEFORE AI INIT");
    ai_anomaly_init();
    ESP_LOGI(TAG, "AFTER AI INIT");
    // Sensor initialization
    ds18b20_init();
    static uint32_t prev_temp_time = 0;
    char data_topic[64];
    snprintf(data_topic,
             sizeof(data_topic),
             "factory/device/%s/data",
             device_id);
    while (1)
    {
        uint32_t timestamp = esp_log_timestamp();

        float temp = ds18b20_read_temp();
        uint32_t temp_time = esp_log_timestamp();
        const char *temp_status;
        const char *temp_color;
        if (prev_temp_time != 0)
        {
            uint32_t Ts_temp = temp_time - prev_temp_time;

            ESP_LOGI(TAG,
                     "TEMP: Ts = %lu ms | Fs = %.4f Hz",
                     Ts_temp,
                     1000.0f / Ts_temp);
        }
        prev_temp_time = temp_time;
        if (temp < -50 || temp > 100)
        {
            temp = 0;
            temp_status = "offline";
            temp_color = "red";
        }
        else
        {
            temp_status = "online";
            temp_color = "green";
        }
        float ph = ph_read();
        float ntu = turbidity_read();
        int ai_result = ai_anomaly_process(temp, ph, ntu);
        if (ai_result == 1)
        {
            ESP_LOGW(TAG, "AI: ANOMALY");
        }
        else if (ai_result == 0)
        {
            ESP_LOGI(TAG, "AI: NORMAL");
        }
        else
        {
            ESP_LOGE(TAG, "AI: ERROR");
        }
        float tur = ntu;
        const char *ph_status;
        const char *ph_color;
        const char *tur_status;
        const char *tur_color;
        const char *ai_status;
        sensor_status_t ph_s = ph_get_status();
        sensor_status_t ntu_s = turbidity_get_status();
        if (ph_s != SENSOR_OK)
        {
            ph = 0;
            ph_status = "offline";
            ph_color = "red";
        }
        else
        {
            ph_status = "online";
            ph_color = "green";
        }
        if (ntu_s != SENSOR_OK)
        {
            tur = 0;
            tur_status = "offline";
            tur_color = "red";
        }
        else
        {
            tur_status = "online";
            tur_color = "green";
        }
        if (isnan(tur))
        {
            tur = 0;
            tur_status = "offline";
            tur_color = "red";
        }
        else
        {
            tur_status = "online";
            tur_color = "green";
        }

        if (ai_result == 1)
        {
            ai_status = "ANOMALY";
        }
        else
        {
            ai_status = "NORMAL";
        }
        static char payload[350];
        snprintf(payload,
                 sizeof(payload),
                 "{"
                 "\"device_id\":\"%s\","
                 "\"timestamp\":%lu,"
                 "\"temp\":%.2f,"
                 "\"ph\":%.2f,"
                 "\"tur\":%.2f,"
                 "\"status\":{"
                     "\"temp\":\"%s\","
                     "\"ph\":\"%s\","
                     "\"tur\":\"%s\""
                 "},"
                 "\"color\":{"
                     "\"temp\":\"%s\","
                     "\"ph\":\"%s\","
                     "\"tur\":\"%s\""
                 "},"
                 "\"ai_status\":\"%s\""
                 "}",
                 device_id,
                 timestamp,
                 temp,
                 ph,
                 tur,
                 temp_status,
                 ph_status,
                 tur_status,
                 temp_color,
                 ph_color,
                 tur_color,
                 ai_status);
        ESP_LOGI(TAG,
                 "TEMP: %.2f (%s) | PH: %.2f (%s) | TUR: %.2f (%s)",
                 temp, temp_status,
                 ph, ph_status,
                 tur, tur_status);
       if (mqtt_is_connected())
{
    mqtt_publish(data_topic, payload);
    
}

else
{
    ESP_LOGW(TAG, "MQTT not ready");
}

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}