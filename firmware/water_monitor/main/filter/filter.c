#include "filter.h"
//khởi tạo trung bình trượt hàm mũ
void ema_init(ema_filter_t *f, float alpha) {
    f->alpha = alpha;
    f->initialized = 0;
}

float ema_update(ema_filter_t *f, float new_value) {
    if (!f->initialized) {
        f->value = new_value;
        f->initialized = 1;
    } else {
        f->value = f->alpha * new_value + (1 - f->alpha) * f->value;
    }
    return f->value;
}

float median3(float a, float b, float c) {
    if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
    if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
    return c;
}