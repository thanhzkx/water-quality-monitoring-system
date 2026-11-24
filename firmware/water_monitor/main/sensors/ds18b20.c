#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <math.h>

#define DS18B20_GPIO GPIO_NUM_4

static const char *TAG = "DS18B20";
//LOW LEVEL
static void ds18b20_set_output(void)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
}

static void ds18b20_set_input(void)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
}

static void ds18b20_write_bit(int bit)
{
    ds18b20_set_output();
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    ds18b20_set_input();
    esp_rom_delay_us(bit ? 64 : 10);
}

static int ds18b20_read_bit(void)
{
    int bit;
    ds18b20_set_output();
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(6);
    ds18b20_set_input();
    esp_rom_delay_us(9);
    bit = gpio_get_level(DS18B20_GPIO);
    esp_rom_delay_us(55);
    return bit;
}

static void ds18b20_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        ds18b20_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte |= (ds18b20_read_bit() << i);
    }
    return byte;
}

static int ds18b20_reset(void)
{
    int presence;
    ds18b20_set_output();
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(480);
    ds18b20_set_input();
    esp_rom_delay_us(70);
    presence = gpio_get_level(DS18B20_GPIO);
    esp_rom_delay_us(410);
    return presence == 0;
}

void ds18b20_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DS18B20_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "DS18B20 init on GPIO %d", DS18B20_GPIO);
}

float ds18b20_read_temp(void)
{
    if (!ds18b20_reset())
    {
        ESP_LOGE(TAG, "Sensor not detected");
        return -1000;
    }
    ds18b20_write_byte(0xCC); // skip ROM
    ds18b20_write_byte(0x44); // convert
    vTaskDelay(pdMS_TO_TICKS(750));
    ds18b20_reset();
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE);
    uint8_t lsb = ds18b20_read_byte();
    uint8_t msb = ds18b20_read_byte();
    int16_t temp_raw = (msb << 8) | lsb;
    float temp = temp_raw / 16.0;
    ESP_LOGI(TAG, "Temperature: %.2f C", temp);
    if (temp == -127 || temp == 85) {
        ESP_LOGW("DS18B20", "Sensor DISCONNECTED");
        return NAN;
    }
    return temp;
}