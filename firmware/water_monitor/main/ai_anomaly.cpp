#include "ai_anomaly.h"
#include "final_autoencoder_int8.h"
#include <math.h>
#include <stdint.h>
#include "esp_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "AI";

static constexpr int WINDOW_SIZE = 10;
static constexpr int FEATURES = 3;
static constexpr float THRESHOLD = 1.1169416074975378f;

// Tham số chuẩn hóa
static constexpr float TEMP_MEAN = 29.61822831f;
static constexpr float TEMP_SCALE = 0.84187071f;
static constexpr float PH_MEAN = 6.89432335f;
static constexpr float PH_SCALE = 0.03347407f;
static constexpr float TURBIDITY_MEAN = 0.10144628f;
static constexpr float TURBIDITY_SCALE = 0.12054617f;

// Trọng số đặc trưng
static constexpr float TEMP_WEIGHT = 0.2f;
static constexpr float PH_WEIGHT = 0.4f;
static constexpr float TURBIDITY_WEIGHT = 0.4f;

// Tham số lượng tử hóa đầu vào
static constexpr float INPUT_SCALE = 0.0225848201662302f;
static constexpr int INPUT_ZERO_POINT = 10;

// Tham số lượng tử hóa đầu ra
static constexpr float OUTPUT_SCALE = 0.020113211125135422f;
static constexpr int OUTPUT_ZERO_POINT = 47;

static const tflite::Model *model = nullptr;
static tflite::MicroInterpreter *interpreter = nullptr;
static TfLiteTensor *input_tensor = nullptr;
static TfLiteTensor *output_tensor = nullptr;

static constexpr int TENSOR_ARENA_SIZE = 64 * 1024;
static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

static float raw_window_data[WINDOW_SIZE][FEATURES];
static float window_data[WINDOW_SIZE][FEATURES];
static int window_count = 0;

static tflite::MicroMutableOpResolver<10> resolver;

void ai_anomaly_init(void)
{
    ESP_LOGI(TAG, "========== AI INIT START ==========");

    // Nạp mô hình
    model = tflite::GetModel(final_autoencoder_int8);

    if (model == nullptr)
    {
        ESP_LOGE(TAG, "Failed to load model");
        return;
    }

    ESP_LOGI(TAG, "Model loaded");

    // Kiểm tra phiên bản mô hình
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        ESP_LOGE(
            TAG,
            "Model schema mismatch: model=%lu expected=%lu",
            (unsigned long)model->version(),
            (unsigned long)TFLITE_SCHEMA_VERSION
        );
        return;
    }

    ESP_LOGI(TAG, "Model schema OK");

    // Đăng ký toán tử
    if (resolver.AddFullyConnected() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddFullyConnected failed");
        return;
    }

    if (resolver.AddReshape() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddReshape failed");
        return;
    }

    if (resolver.AddShape() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddShape failed");
        return;
    }

    if (resolver.AddStridedSlice() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddStridedSlice failed");
        return;
    }

    if (resolver.AddPack() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddPack failed");
        return;
    }

    if (resolver.AddRelu() != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AddRelu failed");
        return;
    }

    // Tạo bộ thông dịch
    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE
    );

    interpreter = &static_interpreter;

    TfLiteStatus status = interpreter->AllocateTensors();

    if (status != kTfLiteOk)
    {
        ESP_LOGE(TAG, "AllocateTensors failed with status=%d", status);
        interpreter = nullptr;
        return;
    }

    ESP_LOGI(TAG, "AllocateTensors OK");

    // Lấy tensor đầu vào và đầu ra
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);

    if (input_tensor == nullptr || output_tensor == nullptr)
    {
        ESP_LOGE(TAG, "Input/output tensor is NULL");
        interpreter = nullptr;
        return;
    }

    ESP_LOGI(
        TAG,
        "Input bytes: %lu",
        (unsigned long)input_tensor->bytes
    );

    ESP_LOGI(
        TAG,
        "Output bytes: %lu",
        (unsigned long)output_tensor->bytes
    );

    ESP_LOGI(TAG, "AI anomaly detector ready");
    ESP_LOGI(TAG, "Window size: %d", WINDOW_SIZE);
    ESP_LOGI(TAG, "Threshold: %.9f", THRESHOLD);

    window_count = 0;

    ESP_LOGI(TAG, "========== AI INIT END ==========");
}

static float scale_temperature(float value)
{
    return (value - TEMP_MEAN) / TEMP_SCALE;
}

static float scale_ph(float value)
{
    return (value - PH_MEAN) / PH_SCALE;
}

static float scale_turbidity(float value)
{
    return (value - TURBIDITY_MEAN) / TURBIDITY_SCALE;
}

static int8_t quantize_int8(float value)
{
    float q = value / INPUT_SCALE + (float)INPUT_ZERO_POINT;
    int32_t rounded = (int32_t)roundf(q);

    if (rounded > 127)
    {
        rounded = 127;
    }

    if (rounded < -128)
    {
        rounded = -128;
    }

    return (int8_t)rounded;
}

static float dequantize_output(int8_t value)
{
    return ((float)value - (float)OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
}

int ai_anomaly_process(
    float temperature,
    float ph,
    float turbidity
)
{
    if (interpreter == nullptr ||
        input_tensor == nullptr ||
        output_tensor == nullptr)
    {
        ESP_LOGE(TAG, "AI is not initialized");
        return -1;
    }

    if (window_count < WINDOW_SIZE)
    {
        // Lưu mẫu mới
        raw_window_data[window_count][0] = temperature;
        raw_window_data[window_count][1] = ph;
        raw_window_data[window_count][2] = turbidity;

        // Chuẩn hóa dữ liệu
        window_data[window_count][0] =
            scale_temperature(temperature);

        window_data[window_count][1] =
            scale_ph(ph);

        window_data[window_count][2] =
            scale_turbidity(turbidity);

        window_count++;
    }
    else
    {
        // Dịch cửa sổ
        for (int i = 0; i < WINDOW_SIZE - 1; i++)
        {
            for (int j = 0; j < FEATURES; j++)
            {
                raw_window_data[i][j] =
                    raw_window_data[i + 1][j];

                window_data[i][j] =
                    window_data[i + 1][j];
            }
        }

        // Thêm mẫu mới
        raw_window_data[WINDOW_SIZE - 1][0] = temperature;
        raw_window_data[WINDOW_SIZE - 1][1] = ph;
        raw_window_data[WINDOW_SIZE - 1][2] = turbidity;

        window_data[WINDOW_SIZE - 1][0] =
            scale_temperature(temperature);

        window_data[WINDOW_SIZE - 1][1] =
            scale_ph(ph);

        window_data[WINDOW_SIZE - 1][2] =
            scale_turbidity(turbidity);
    }

    // Chờ đủ 10 mẫu
    if (window_count < WINDOW_SIZE)
    {
        ESP_LOGI(
            TAG,
            "AI window: %d/%d",
            window_count,
            WINDOW_SIZE
        );

        return 0;
    }

    // Chuẩn bị dữ liệu INT8
    int index = 0;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        for (int j = 0; j < FEATURES; j++)
        {
            input_tensor->data.int8[index++] =
                quantize_int8(window_data[i][j]);
        }
    }

    ESP_LOGI(TAG, "========== AI INT8 INPUT ==========");

    index = 0;

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        ESP_LOGI(
            TAG,
            "W[%d] INT8: T=%d PH=%d TUR=%d",
            i,
            input_tensor->data.int8[index],
            input_tensor->data.int8[index + 1],
            input_tensor->data.int8[index + 2]
        );

        index += FEATURES;
    }

    ESP_LOGI(TAG, "========== AI INPUT WINDOW ==========");

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        ESP_LOGI(
            TAG,
            "W[%d] raw: T=%.4f PH=%.4f TUR=%.4f | scaled: T=%.4f PH=%.4f TUR=%.4f",
            i,
            raw_window_data[i][0],
            raw_window_data[i][1],
            raw_window_data[i][2],
            window_data[i][0],
            window_data[i][1],
            window_data[i][2]
        );
    }

    // Chạy suy luận
    TfLiteStatus status = interpreter->Invoke();

    if (status != kTfLiteOk)
    {
        ESP_LOGE(TAG, "Inference failed");
        return -1;
    }

    // Tính sai số tái tạo
    float feature_error_sum[FEATURES] = {
        0.0f,
        0.0f,
        0.0f
    };

    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        for (int j = 0; j < FEATURES; j++)
        {
            int index = i * FEATURES + j;

            float original =
                window_data[i][j];

            float reconstructed =
                dequantize_output(
                    output_tensor->data.int8[index]
                );

            float diff =
                original - reconstructed;

            feature_error_sum[j] +=
                diff * diff;
        }
    }

    // Sai số trung bình của từng đặc trưng
    float temp_error =
        feature_error_sum[0] / WINDOW_SIZE;

    float ph_error =
        feature_error_sum[1] / WINDOW_SIZE;

    float turbidity_error =
        feature_error_sum[2] / WINDOW_SIZE;

    // Sai số tái tạo có trọng số
    float reconstruction_error =
          TEMP_WEIGHT * temp_error
        + PH_WEIGHT * ph_error
        + TURBIDITY_WEIGHT * turbidity_error;

    ESP_LOGI(
        TAG,
        "AI error: total=%.6f | T=%.6f PH=%.6f TUR=%.6f",
        reconstruction_error,
        temp_error,
        ph_error,
        turbidity_error
    );

    // Phân loại bất thường
    if (reconstruction_error > THRESHOLD)
    {
        ESP_LOGW(
            TAG,
            "ANOMALY detected! error=%.6f threshold=%.6f",
            reconstruction_error,
            THRESHOLD
        );

        return 1;
    }

    ESP_LOGI(
        TAG,
        "NORMAL error=%.6f threshold=%.6f",
        reconstruction_error,
        THRESHOLD
    );

    return 0;
}