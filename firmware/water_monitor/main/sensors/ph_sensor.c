#include "ph_sensor.h"
#include "adc_utils.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "filter.h"
#include "sensor_status.h"

#define PH_CHANNEL ADC_CHANNEL_0  // GPIO 2 ESP32-C3

// Giá trị pH hợp lệ cuối cùng
static float last_valid_ph = 6.86;
static sensor_status_t ph_status = SENSOR_OK;
static const char *TAG = "PH_SENSOR";
static ema_filter_t ph_ema;
static int ph_filter_initialized = 0;

// Điện áp tại pH 4.01
#define VREF_PH 1.360f

// Hệ số bộ chia áp
#define DIVIDER_GAIN 2.0f

// Độ dốc cảm biến
#define PH_SLOPE 0.03544f

float ph_read(void)
{
    if (!ph_filter_initialized) {
        ema_init(&ph_ema, 0.2f);
        ph_filter_initialized = 1;
    }
    float sum = 0;
    // Lấy 30 mẫu để tính trung bình
    uint32_t first_time = 0;
    uint32_t last_time = 0;
    for (int i = 0; i < 30; i++) {
        uint32_t t = esp_log_timestamp();
        if (i == 0)
            first_time = t;
        sum += adc_read_voltage(PH_CHANNEL);
        last_time = t;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    uint32_t total_time = last_time - first_time;
    float Ts_ph = (float)total_time / 29.0f;
    float Fs_ph = 1000.0f / Ts_ph;
    ESP_LOGI(TAG,
             "PH ADC: Ts = %.3f ms | Fs = %.3f Hz",
             Ts_ph,
             Fs_ph);
    float voltage = sum / 30.0f;
    // Không sử dụng bộ lọc trung vị
    float voltage_med = voltage;
    // Kiểm tra cảm biến mất kết nối
    if (voltage_med < 0.1f || voltage_med > 2.8f) {
        ESP_LOGW(TAG, "pH sensor disconnected! (V_adc = %.3fV)", voltage_med);
        ph_status = SENSOR_DISCONNECTED;
        return last_valid_ph;
    }
 float sensor_voltage = voltage_med * DIVIDER_GAIN;
float ph = 4.01f + ((VREF_PH - sensor_voltage) / PH_SLOPE);
    // Giới hạn pH trong khoảng 0 đến 14
    if (ph < 0.0f) ph = 0.0f;
    if (ph > 14.0f) ph = 14.0f;
    // Lọc EMA
    //float ph_filtered = ema_update(&ph_ema, ph);
    float ph_filtered = ph;
    // Kiểm tra phạm vi pH
    if (ph_filtered < 0.0f || ph_filtered > 14.0f) {
        ESP_LOGW(TAG, "Invalid pH calculated: %.2f", ph_filtered);
        ph_status = SENSOR_ERROR;
        return last_valid_ph;
    }
    // Loại bỏ thay đổi pH quá lớn
    /*
if (fabs(ph_filtered - last_valid_ph) > 1.5f) {
    ...
}
*/
    if (fabs(ph_filtered - last_valid_ph) > 10.0f) {
        ESP_LOGW(TAG,
                 "pH spike detected! Old: %.2f -> New: %.2f (Ignored)",
                 last_valid_ph,
                 ph_filtered);
        ph_filtered = last_valid_ph;
    } else {
        ph_status = SENSOR_OK;
    }
    // Cập nhật giá trị cuối
    last_valid_ph = ph_filtered;
    ESP_LOGI(TAG,
             "ADC=%.3fV | SENSOR=%.3fV | pH_raw=%.2f | pH=%.2f",
             voltage_med,
             sensor_voltage,
             ph,
             ph_filtered);
    return ph_filtered;
}

sensor_status_t ph_get_status(void)
{
    return ph_status;
}