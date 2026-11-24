#ifndef MQTT_H
#define MQTT_H

void mqtt_start(void);
void mqtt_publish(const char *topic, const char *data);
int mqtt_is_connected(void);

#endif