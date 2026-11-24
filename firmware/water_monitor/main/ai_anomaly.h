#ifndef AI_ANOMALY_H
#define AI_ANOMALY_H

#ifdef __cplusplus
extern "C" {
#endif

void ai_anomaly_init(void);

int ai_anomaly_process(
    float temperature,
    float ph,
    float turbidity
);

#ifdef __cplusplus
}
#endif

#endif