#include "display.h"

#include <LEDMatrixDriver.hpp>
#include "wmconfig.h"
#include "font.h"

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

void getScrolltextFromBuffer(int channel)
{
  LogTarget.print((String)"Show buffer " + channel + ": [");
  bool eot = false;
  for (int i = 0; i < MAX_TEXT_LENGTH; i++) {
    if (eot) {
      scrollbuffer[i] = 0;
    } else {
      uint8_t ch = texts[channel][i];
      scrollbuffer[i] = ch;
      if (i <= 30)
        LogTarget.print(String(char(ch)));
      if (!ch) {
        eot = true;
        LogTarget.println((String)"] (" + i + " bytes)");
      }
    }
  }
  scrollbuffer[MAX_TEXT_LENGTH - 1] = 0;
}

void showStatus(char* text)
{
  Serial.println((String)"showStatus: " + text);
  textIndex = 0;
  current_channel = 0;
  sprintf((char*)texts[current_channel], "%s", text);
  getScrolltextFromBuffer(current_channel);
  colIndex = 0;
  marqueeDelayTimestamp = 0;
  scrollWhitespace = 0;
  led->clear();
  uint8_t displayColumn = 0; // start left
  while (displayColumn < ledmatrix_width) {
    // write one column
    uint8_t asc = scrollbuffer[textIndex] - 32;
    uint16_t idx = pgm_read_word(&(font_index[asc]));
    uint8_t w = pgm_read_byte(&(font[idx]));
    uint8_t col = pgm_read_byte(&(font[colIndex + idx + 1]));
    led->setColumn(displayColumn, col);
    led->display();
    displayColumn++;
    if (++colIndex == w) {
      displayColumn += 1;
      colIndex = 0;
      if (scrollbuffer[++textIndex] == '\0') {
        break;  //return; // done
        textIndex = 0;
      }
    }
  }
  delay(500);
}


void nextChar()
{
  if (scrollbuffer[++textIndex] == '\0')
  {
    textIndex = 0;
    scrollWhitespace = ledmatrix_width; // start over with empty display
    if (scrollDelay && do_publishes)
      client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "repeat");

    if (start_countdown) {
      start_countdown--;
    } else {
      current_channel = textcycle[current_cycle];
      current_cycle = (current_cycle + 1) % num_textcycles;
      getScrolltextFromBuffer(current_channel);
    }
  }
}

void nextCol(uint8_t w)
{
  if (++colIndex == w)
  {
    scrollWhitespace = 2;
    colIndex = 0;
    nextChar();
  }
}

void writeCol()
{
  if (scrollWhitespace > 0)
  {
    scrollWhitespace--;
    return;
  }
  uint8_t asc = scrollbuffer[textIndex] - 32;
  uint16_t idx = pgm_read_word(&(font_index[asc]));
  uint8_t w = pgm_read_byte(&(font[idx]));
  uint8_t col = pgm_read_byte(&(font[colIndex + idx + 1]));
  led->setColumn(ledmatrix_width - 1, col);
  nextCol(w);
}

void marquee()
{
  if (millis() < 1)
    marqueeDelayTimestamp = 0;
  if (millis() < marqueeDelayTimestamp)
    return;
  marqueeDelayTimestamp = millis() + scrollDelay;
  led->scroll(LEDMatrixDriver::scrollDirection::scrollLeft);
  writeCol();
  led->display();
}