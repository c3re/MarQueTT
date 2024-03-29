#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "local_config.h"
#include "LEDMatrixDriver.hpp"
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

uint8_t scrollbuffer[MAX_TEXT_LENGTH];

LEDMatrixDriver* led;
uint8_t texts[NUM_CHANNELS][MAX_TEXT_LENGTH];
uint8_t textcycle[MAX_TEXTCYCLE];
uint8_t num_textcycles = 1;
uint8_t current_cycle = 0;
uint8_t start_countdown = START_CYCLES;
uint8_t colIndex = 0;
uint16_t current_channel = 0;
uint16_t textIndex = 0;
uint16_t scrollWhitespace = 0;
uint16_t ledmatrix_width = INITIAL_LEDMATRIX_SEGMENTS * 8;
uint16_t blinkDelay = 0;
uint64_t marqueeCycleTimestamp = 0;
uint64_t marqueeDelayTimestamp = 0;
uint64_t marqueeBlinkTimestamp;

WiFiClient espClient;
PubSubClient client(espClient);

void getScrolltextFromBuffer(int channel);
void showStatus(char* text);
void nextChar();
void nextCol(uint8_t w);
void writeCol();
void marquee();

#endif