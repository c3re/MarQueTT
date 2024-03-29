#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include "local_config.h"
#include <LEDMatrixDriver.hpp>
#include "display.h"

void printHex8(uint8_t *data, uint8_t length);
void mqttReceiveCallback(char* topic, byte* payload, unsigned int length);

#endif