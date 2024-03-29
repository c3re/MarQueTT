/*
    MarQueTT[ino]     Matrix Display based on n x 8x8 monochrome LEDs, driven by MAX7221

    a c3RE project
*/

#define VERSION "1.5.7"

#include "local_config.h"
#include "wmconfig.h"
#include "font.h"
#include "mqtt.h"
#include "display.h"

//TODO: globalisieren von devname und font_index

// variables
bool first_connect = true;


// function prototypes
void loop_matrix();
void reconnect();

// functions
void setup() {
  led = new LEDMatrixDriver(INITIAL_LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, 0);
  led->setIntensity(0);
  led->setEnabled(true);
  for (int y = 0; y < 8; y++)
    led->setPixel(63, y, 1);
  led->display();

  for (int c = 0; c < NUM_CHANNELS; c++) {
    //snprintf((char*)(texts[c]), MAX_TEXT_LENGTH, "[%d] %s", c, initialText);
    snprintf((char*)(texts[c]), sizeof(texts[c]), "MarQueTTino %s Version %s", devaddr, VERSION);
  }
  textcycle[0] = 0;

  calculate_font_index();

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println();
  Serial.println("MarQueTT[ino] Version " VERSION);
  Serial.println();
      EEPROM.begin(512);
      checkzweiterstart();
      dumpEEPROMBuffer();
  setup_wifi(showStatus);
  ledmatrix_width = String(sett.num_segments).toInt() * 8;
  free(led);
  led = new LEDMatrixDriver(String(sett.num_segments).toInt(), LEDMATRIX_CS_PIN, 0);
  led->setIntensity(0);
  led->setEnabled(true);
  for (int y = 0; y < 8; y++)
    led->setPixel(63, y, 1);
  led->display();
  client.setServer(sett.mqtt_broker, 1883);
  client.setCallback(mqttReceiveCallback);
  client.setBufferSize(MAX_TEXT_LENGTH);
}

void loop()
{
  ArduinoOTA.handle();
  if (blinkDelay) {
    if (marqueeBlinkTimestamp + blinkDelay < millis()) {
      led->setEnabled(false);
      delay(1);
      marqueeBlinkTimestamp = millis();
    } else if (marqueeBlinkTimestamp + blinkDelay / 2 < millis()) {
      led->setEnabled(true);
      delay(1);
    }
  }
  loop_matrix();
  if (!client.connected()) {
    reconnect();
    showStatus("ready");
    if (do_publishes)
      if (first_connect) {
        client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "startup");
        client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), ((String)"version " + VERSION).c_str());
        client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), ((String)"ip " + ip.toString()).c_str());
        first_connect = false;
      }
    client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "reconnect");
  }
  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    LogTarget.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), sett.mqtt_user, sett.mqtt_password, (((String)TOPICROOT "/" + devname + "/status").c_str()), 1, true, "offline")) {
      LogTarget.println("connected");
      client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "online", true);
      client.subscribe(TOPICROOT "/#");
      showStatus("MQTT ok");
    } else {
      LogTarget.print("failed, rc=");
      LogTarget.print(client.state());
      LogTarget.println(" try again in 5 seconds");
      showStatus("MQTT error");
      delay(5000);
    }
  }
}

void loop_matrix()
{
  if (scrollDelay) {
    marquee();
  } else {

    // cycle non-scrolling text channels
    if (marqueeCycleTimestamp + cycleDelay < millis()) {
      marqueeCycleTimestamp = millis();
      textIndex = 0;
      LogTarget.println("next static text");
      current_channel = textcycle[current_cycle];
      current_cycle = (current_cycle + 1) % num_textcycles;
    }

    if (textIndex == 0) {   // begin writing text to display (e.g. after text was changed)
      LogTarget.println((String)"Display static text no." + current_channel);
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
            return; // done
            textIndex = 0;
          }
        }
      }
    }
  }
}
