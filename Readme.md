# MarQueTT[ino]

## A MAX7221 LED-Matrix Scrolling Marquee controlled via MQTT

### MQTT Topics

#### Global Topics

##### Subscribed by Device

- `ledMatrix/all/text`  : A UTF-8 coded text, max. 4096 bytes long.
- `ledMatrix/all/intensity`: 0 = lowest, 15 = highest. Default: 1.
- `ledMatrix/all/delay`: 1 = fastest, 1000 = slowest scrolling. Default: 25 
-  &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; 0 = no scrolling; < 0: negative value sets page cycle time in ms. Default: 5000 ms
- `ledMatrix/all/blink`: 0 = no blinking; 1 = fastest, 1000 = slowest blinking. Default: 0
- `ledMatrix/all/enable`: 0 = display off, 1 = display on. Default: 1

Default values are configurable in `local_config.h`. You can also use retained messages, preferably with individual topics (see below).

**Extension:** text channels, `<cno>` = `0`..`9`

- `ledMatrix/all/text/<cno>`    — text for a specific channel (`.../text/0` = same as `.../text`)
- `ledMatrix/all/channel`       — set channels to be displayed
  - `<cno1>[,<cno2>...]`    — list of channels
  - `<cno1>`                — only one channel
  - "" (empty)              — only channel 0


#### Individual Topics

##### Subscribed by Device

Same as global topics, but `ledMatrix/MarQueTTino-<aabbcc>/...` instead of `ledMatrix/all/...` (<aabbcc> == serial number in 3 hex bytes).

TBD: priority of individual topics over global topics ?

##### Published by Device

- `ledMatrix/MarQueTTino-<aabbcc>/status`:
  - `startup`       — sent on system start (after first MQTT connect)
  - `version x.y.z` — sent on system start (after first MQTT connect): firmware version number
  - `ip w.x.y.z`    — sent on system start (after first MQTT connect): IP address
  - `reconnect`     — sent on MQTT reconnect
  - `repeat`        — sent at end of a sequence (TBD: for each channel or total sequence?)
  - `offline`       — sent as will when the MQTT connection disconnects unexpectedly

### Setup

#### Wiring

Connect your display's X pin to your controller's Y pin:

- `Data IN` to `MOSI`
- `CLK` to `SCK`
- `CS` to any digial port except `MISO` or `SS` (e.g. `D4`) 

See https://github.com/bartoszbielawski/LEDMatrixDriver#pin-selection for more information

There may be problems when the power supply to the matrix is not good. Usually, the 5V output of the WEMOS module is connected simply to the matrix input connector, but it might be necessary to
- connect to other power inputs on the matrix, too – especially when several modules (3+) are chained,
- have an "angst" capacitor between +5V and GND, or even
- have an extra power supply (not via USB/WEMOS). Remember that the WEMOS has a maximum current it can route from the USB supply to the +5V output pin. If this is exceeded, the SMD diode in between may burn out.

#### Software

- Use Arduino or compatible environment with the ESP8266 board package installed.
- Have the following libraries installed (versions given here are known to work):
    - LEDMatrixDriver by Bartosz Bielawski (version 0.2.2),
    - PubSubClient by Nick O'Leary (version 2.8),
    - WiFiManager by tablatronix (version 2.0.12-beta).
- Copy `local_config.dist.h` to `local_config.h` and change settings if really needed.
    - WiFi and MQTT setup is done via web interface. 
    - Check the `LEDMATRIX_`* constants according to your hardware setup.
    - You might also want to change the default values.

#### Setup via WiFiManager

When the device is started for the first time, it creates an WiFi access point with the SSID name of "MarQueTTino-<aabbcc>", <aabbcc> being the serial number as described above. When you connect to that network with your computer, it will display a setup dialogue provided by the WiFiManager library. Here you can choose the WiFi it should connect to, give the password and finally set the MQTT parameters: broker hostname, and optionally MQTT user and password. These parameters are then stored in the ESP's flash memory. If the device cannot join the configured WiFi network on startup anytime later, it will again present the AP.

When the device is started, it gives several status messages on the display. While a message "reset?" is shown, the reset button can be pressed in order to force a factory default reset. After this, the device will again go into AP mode and the setup can / must be done. Without reset, the AP is always shown when the network + MQTT access is not possible.

The AP menu can also be used in order to install a *.bin (firmware binary) file for an update.

### Change Ideas

- perhaps implement a priority function so that device-specific topics precede the general topics.

### Extension Ideas

- WS2812 LED (one or more?) for quick signalling
- acoustic output via piezo element for signalling
- some push buttons for remote feedback or for local display control (cycle next, acknowledge, etc.)
- use an IR receiver together with a spare remote control

