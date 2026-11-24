#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "sensor_status.h"

float ph_read(void);
sensor_status_t ph_get_status(void);

#endif