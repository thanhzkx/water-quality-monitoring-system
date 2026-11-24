#include "mqtt.h"

#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_mac.h"
#include "wifi_manager.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;
static int mqtt_connected = 0;


/* =========================================================
 * MQTT EVENT
 * ========================================================= */

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(TAG, "MQTT CONNECTED");

            mqtt_connected = 1;

            /*
             * ESP32 bắt đầu nghe topic cấu hình WiFi
             */
            int msg_id = esp_mqtt_client_subscribe(
                client,
                "factory/device/+/wifi/config",
                1
            );

            ESP_LOGI(
                TAG,
                "Subscribed WiFi config | msg_id=%d",
                msg_id
            );

            break;


        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGI(TAG, "MQTT DISCONNECTED");

            mqtt_connected = 0;

            break;


        case MQTT_EVENT_DATA:

            ESP_LOGI(TAG, "MQTT DATA RECEIVED");

            ESP_LOGI(
                TAG,
                "Topic: %.*s",
                event->topic_len,
                event->topic
            );

            ESP_LOGI(
                TAG,
                "Data: %.*s",
                event->data_len,
                event->data
            );
            char json[512];

if (event->data_len >= sizeof(json))
{
    ESP_LOGE(TAG, "WiFi config too large");
    break;
}

memcpy(json, event->data, event->data_len);
json[event->data_len] = '\0';

cJSON *root = cJSON_Parse(json);

if (root == NULL)
{
    ESP_LOGE(TAG, "Invalid JSON");
    break;
}

cJSON *action = cJSON_GetObjectItem(root, "action");

if (cJSON_IsString(action) &&
    strcmp(action->valuestring, "delete") == 0)
{
    ESP_LOGI(TAG, "Delete WiFi configuration");

    wifi_delete_config();

    cJSON_Delete(root);
    break;
}

cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
cJSON *password = cJSON_GetObjectItem(root, "password");

if (!cJSON_IsString(ssid) ||
    !cJSON_IsString(password))
{
    ESP_LOGE(TAG, "Invalid WiFi credentials");

    cJSON_Delete(root);
    break;
}

ESP_LOGI(
    TAG,
    "New WiFi SSID: %s",
    ssid->valuestring
);

wifi_save_and_connect(
    ssid->valuestring,
    password->valuestring
);

cJSON_Delete(root);

break;


        default:
            break;
    }
}


/* =========================================================
 * MQTT START
 * ========================================================= */

void mqtt_start(void)
{
    uint8_t mac[6];

    ESP_ERROR_CHECK(
        esp_read_mac(mac, ESP_MAC_WIFI_STA)
    );

    char client_id[32];

    snprintf(
        client_id,
        sizeof(client_id),
        "ESP32C3_%02X%02X%02X%02X%02X%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    ESP_LOGI(
        TAG,
        "MQTT Client ID: %s",
        client_id
    );


    esp_mqtt_client_config_t config = {
        .broker.address.uri =
            "mqtt://180.93.228.86:1883",

        .credentials.client_id =
            client_id
    };


    client =
        esp_mqtt_client_init(&config);


    esp_mqtt_client_register_event(
        client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );


    esp_mqtt_client_start(client);
}


/* =========================================================
 * MQTT PUBLISH
 * ========================================================= */

void mqtt_publish(
    const char *topic,
    const char *data)
{
    if (client == NULL)
    {
        ESP_LOGW(
            TAG,
            "MQTT client is NULL"
        );

        return;
    }

    int msg_id =
        esp_mqtt_client_publish(
            client,
            topic,
            data,
            0,
            1,
            0
        );

    ESP_LOGI(
        TAG,
        "Publish topic=%s | msg_id=%d",
        topic,
        msg_id
    );
}


/* =========================================================
 * MQTT STATUS
 * ========================================================= */

int mqtt_is_connected(void)
{
    return mqtt_connected;
}