#include "adc_utils.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC_UTILS";
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle = NULL;

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &config)); // GPIO1
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
        &cali_config,
        &adc_cali_handle
    ));

    ESP_LOGI(TAG, "ADC initialized + calibrated");
}

float adc_read_voltage(int channel)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &raw));

    int voltage_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(
        adc_cali_handle,
        raw,
        &voltage_mv
    ));

    float voltage = voltage_mv / 1000.0;

    return voltage;
}