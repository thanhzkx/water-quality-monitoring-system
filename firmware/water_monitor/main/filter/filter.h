#ifndef FILTER_H
#define FILTER_H

typedef struct {
    float alpha;
    float value;
    int initialized;
} ema_filter_t;

void ema_init(ema_filter_t *f, float alpha);
float ema_update(ema_filter_t *f, float new_value);

float median3(float a, float b, float c);

#endif