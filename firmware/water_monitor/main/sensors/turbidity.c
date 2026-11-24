#include "turbidity.h"
#include "adc_utils.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "filter.h"
#include "sensor_status.h"

#define TURBIDITY_CHANNEL ADC_CHANNEL_1
#define SCALE 2

// Giá trị NTU hợp lệ cuối cùng
static float last_valid_ntu = 0;
static sensor_status_t ntu_status = SENSOR_OK;
static const char *TAG = "TURBIDITY";
static ema_filter_t ntu_ema;
static int ntu_filter_initialized = 0;

// Điện áp chuẩn đo thực tế
static float U0 = 4.56f;

// Nội suy theo đặc tuyến cảm biến
static float turbidity_from_f(float f)
{
    if (f >= 0.98)
        return 0;
    else if (f >= 0.72)
        return (0.98 - f) / (0.98 - 0.72) * 500;
    else if (f >= 0.51)
        return 500 + (0.72 - f) / (0.72 - 0.51) * 500;
    else if (f >= 0.31)
        return 1000 + (0.51 - f) / (0.51 - 0.31) * 1000;
    else if (f >= 0.15)
        return 2000 + (0.31 - f) / (0.31 - 0.15) * 2000;
    else
        return 4000;
}

float turbidity_read(void)
{
    if (!ntu_filter_initialized) {
        ema_init(&ntu_ema, 0.1f);
        ntu_filter_initialized = 1;
    }
    float sum = 0;
    uint32_t first_time = 0;
    uint32_t last_time = 0;
    for (int i = 0; i < 10; i++) {
        uint32_t t = esp_log_timestamp();
        if (i == 0)
            first_time = t;
        sum += adc_read_voltage(TURBIDITY_CHANNEL);
        last_time = t;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    uint32_t total_time = last_time - first_time;
    float Ts_tur = (float)total_time / 9.0f;
    float Fs_tur = 1000.0f / Ts_tur;
    ESP_LOGI(TAG,
             "TUR ADC: Ts = %.3f ms | Fs = %.3f Hz",
             Ts_tur,
             Fs_tur);
    float voltage_adc = sum / 10.0;
    float voltage_med = voltage_adc;
    // Kiểm tra cảm biến mất kết nối
    if (voltage_med < 0.1 || voltage_med > 3.2) {
        ESP_LOGW(TAG, "Turbidity sensor disconnected!");
        ntu_status = SENSOR_DISCONNECTED;
        return last_valid_ntu;
    }
    // Khôi phục điện áp thực
    float Um = voltage_med * SCALE;
    float f = Um / U0;
    float ntu = turbidity_from_f(f);
    // Lọc EMA
    float ntu_filtered = ema_update(&ntu_ema, ntu);
    // Kiểm tra phạm vi NTU
    if (ntu_filtered < 0 || ntu_filtered > 4000) {
        ESP_LOGW(TAG, "Invalid NTU!");
        ntu_status = SENSOR_ERROR;
        return last_valid_ntu;
    }
    if (last_valid_ntu > 0 &&
        fabs(ntu_filtered - last_valid_ntu) > 300)
    {
        ESP_LOGW(TAG, "NTU spike!");
        ntu_filtered = last_valid_ntu;
    }
    // Cập nhật giá trị cuối
    last_valid_ntu = ntu_filtered;
    ntu_status = SENSOR_OK;
    ESP_LOGI(TAG,
             "ADC=%.3fV | MED=%.3fV | NTU_raw=%.2f | NTU_filt=%.2f",
             voltage_adc,
             voltage_med,
             ntu,
             ntu_filtered);
    return ntu_filtered;
}

sensor_status_t turbidity_get_status(void)
{
    return ntu_status;
}