#ifndef TURBIDITY_H
#define TURBIDITY_H

#include "sensor_status.h"

float turbidity_read(void);
sensor_status_t turbidity_get_status(void);

#endif