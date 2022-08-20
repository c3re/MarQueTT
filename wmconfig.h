/**
   WiFiManagerParameter child class example
*/
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Arduino.h>
#include <EEPROM.h>


////////////////
// SETUP

// general
#define APPNAME "MarQueTTino"
#define APPVERSION VERSION
//                  ^^^^^------ change APPVERSION in order to invalidate flash storage!
#define IDENT   (APPNAME "_" APPVERSION)

#define SETUP_PIN 0

#define IDENT_LENGTH 32
#define STR_LENGTH 80

struct Settings {
  char settings_identifier[IDENT_LENGTH];
  char mqtt_broker[STR_LENGTH];
  char mqtt_user[STR_LENGTH];
  char mqtt_password[STR_LENGTH];
  //char mqtt_topic[STR_LENGTH];
} sett;

//flag for saving data
bool shouldSaveConfig = false;

char devaddr[20];
char devname[40];
IPAddress ip;
String devname_lc;



//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup_wifi() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  pinMode(SETUP_PIN, INPUT_PULLUP);

  //Delay to push SETUP button
  Serial.println("Press setup button");
  for (int sec = 3; sec > 0; sec--) {
    Serial.print(sec);
    Serial.print("..");
    delay(1000);
  }

  // warning for example only, this will initialize empty memory into your vars
  // always init flash memory or add some checksum bits
  EEPROM.begin( 512 );
  EEPROM.get(0, sett);
  Serial.println("Settings loaded");

  // Use [APPNAME]-[MAC] as Name for (a) WifiManager AP, (b) OTA hostname, (c) MQTT client name
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(devaddr, sizeof(devaddr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  snprintf(devname, sizeof(devname), APPNAME "-%02X%02X%02X", mac[3], mac[4], mac[5]);

  WiFiManagerParameter param_mqtt_broker( "host", "MQTT broker hostname",  sett.mqtt_broker, STR_LENGTH);
  WiFiManagerParameter param_mqtt_user( "user", "MQTT user name",  sett.mqtt_user, STR_LENGTH);
  WiFiManagerParameter param_mqtt_password( "pswd", "MQTT password",  sett.mqtt_password, STR_LENGTH);
  //WiFiManagerParameter param_mqtt_topic( "topc", "MQTT topic",  sett.mqtt_topic, STR_LENGTH);

  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter( &param_mqtt_broker );
  wm.addParameter( &param_mqtt_user );
  wm.addParameter( &param_mqtt_password );
  //wm.addParameter( &param_mqtt_topic );

  if (digitalRead(SETUP_PIN) == LOW) {    // Button pressed
    Serial.println("SETUP");
    wm.startConfigPortal();
  } else {
    Serial.println("WORK");
    wm.autoConnect(devname);
  }


#if LOG_TELNET
  TelnetStream.begin();
  Serial.println((String)"All further logging is routed to telnet. Just connect to " + devname + " port 22.");
#endif

  LogTarget.println((String)"This device is called '" + devname + "'.");
  devname_lc = String(devname);
  devname_lc.toLowerCase(); // used for topic comparisons

  strncpy(sett.mqtt_broker, param_mqtt_broker.getValue(), STR_LENGTH);
  sett.mqtt_broker[STR_LENGTH - 1] = '\0';
  strncpy(sett.mqtt_user, param_mqtt_user.getValue(), STR_LENGTH);
  sett.mqtt_user[STR_LENGTH - 1] = '\0';
  strncpy(sett.mqtt_password, param_mqtt_password.getValue(), STR_LENGTH);
  sett.mqtt_password[STR_LENGTH - 1] = '\0';
  //strncpy(sett.mqtt_broker, param_mqtt_broker.getValue(), STR_LENGTH);
  //sett.mqtt_broker[STR_LENGTH - 1] = '\0';

  LogTarget.println((String)"MQTT broker hostname: " + sett.mqtt_broker);
  LogTarget.println((String)"MQTT user name:       " + sett.mqtt_user);
  LogTarget.println((String)"MQTT password:        " + sett.mqtt_password);
  //LogTarget.println((String)"MQTT topic:           " + sett.mqtt_topic);

  if (shouldSaveConfig) {
    LogTarget.println("Settings changed, need to save them to flash");
    EEPROM.put(0, sett);
    if (EEPROM.commit()) {
      LogTarget.println("Settings saved");
    } else {
      LogTarget.println("EEPROM error");
    }
  }
  LogTarget.println("Done with WiFi Setup! Results:");

  LogTarget.println((String)"MQTT broker hostname: " + sett.mqtt_broker);
  LogTarget.println((String)"MQTT user name:       " + sett.mqtt_user);
  LogTarget.println((String)"MQTT password:        " + sett.mqtt_password);
  //Serial.println((String)"MQTT topic:           " + sett.mqtt_topic);


  //// OTA FUNCTIONS
  //
  //
  ArduinoOTA.setHostname(devname);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    LogTarget.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    LogTarget.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    LogTarget.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    LogTarget.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      LogTarget.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      LogTarget.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      LogTarget.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      LogTarget.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      LogTarget.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  //
  //
  //// END OF OTA FUNCTIONS

}
