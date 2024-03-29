#include "mqtt.h"

#include "wmconfig.h"
#include "font.h"

void printHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
  char tmp[16];
  for (int i = 0; i < length; i++) {
    sprintf(tmp, "0x%.2X", data[i]);
    LogTarget.print(tmp); LogTarget.print(" ");
  }
}

//wtf?
void mqttReceiveCallback(char* topic, byte* payload, unsigned int length)
{
  const bool pr = DEBUGPRINT;                   // set to 1 for debug prints

  if (String(topic).endsWith("status")) return; // don't process stuff we just published

  LogTarget.print((String)"MQTT in: " + topic + "\t = [");
  for (int i = 0; i < min((int)length, 50); i++)  LogTarget.print((char)(payload[i]));
  if (length > 50) LogTarget.print("...");
  LogTarget.println("]");

  String command = topic + String(topic).lastIndexOf(TOPICROOT "/") + strlen(TOPICROOT) + 1;
  command.toLowerCase();

  if (pr) LogTarget.println((String)"Check command " + command + " for match with devname_lc " + devname_lc);

  if (command.startsWith("marquettino-")) { // device-specific topic was used
    if (pr) LogTarget.println((String)"    ok, has prefix 'marquettino-'");
    if (command.startsWith(devname_lc)) {   // this device was addressed
      if (pr) LogTarget.println((String)"    ok, matches our name");
      command.remove(0, strlen(devname) + 1);   // strip device name
    } else {                                // other device => ignore
      if (pr) LogTarget.print((String)"    no match, ignore command");
      return;
    }
  } else if (command.startsWith("all")) {   // all devices
    if (pr) LogTarget.println((String)"    ok, matches 'all'");
    command.remove(0, strlen("all") + 1);   // strip device name
  } else {
    if (pr) LogTarget.println((String)"    incorrect/obsolete addressing scheme, ignore command");
    return;
  }

  LogTarget.println((String)"Command = [" + command + "]");

  if (command.equals("intensity")) {
    int intensity = 0;
    for (int i = 0 ; i < length;  i++) {
      intensity *= 10;
      intensity += payload[i] - '0';
    }
    intensity = max(0, min(15, intensity));
    led->setIntensity(intensity);
    return;
  }

  if (command.equals("delay")) {
    scrollDelay = 0;
    bool negative = false;
    int i = 0;
    if (payload[0] == '-') {
      negative = true;
      i = 1;
    }
    for ( ; i < length;  i++) {
      scrollDelay *= 10;
      scrollDelay += payload[i] - '0';
    }

    if (scrollDelay == 0) {
      textIndex = 0;
      LogTarget.println((String)"Set no-scroll, cycle = " + cycleDelay);
    } else if (negative) {
      cycleDelay = scrollDelay;   // negative value given is new cycle delay, no scrolling
      textIndex = 0;
      scrollDelay = 0;
      LogTarget.println((String)"Set no-scroll, cycle = " + cycleDelay);
    } else {
      if (scrollDelay > 10000) {
        scrollDelay = 10000;
      }
      LogTarget.println((String)"Set scroll delay = " + scrollDelay);
    }
    return;
  }

  if (command.equals("blink")) {
    blinkDelay = 0;
    for (int i = 0 ; i < length;  i++) {
      blinkDelay *= 10;
      blinkDelay += payload[i] - '0';
    }
    if (blinkDelay < 0) {
      blinkDelay = 0;
    }
    if (blinkDelay > 10000) {
      blinkDelay = 10000;
    }
    if (!blinkDelay) {
      led->setEnabled(true);
    } else {
      marqueeBlinkTimestamp = millis();
    }

    return;
  }
  if (command.equals("reset")) {
  
      ESP.restart();

  }


  if (command.equals("enable")) {
    led->setEnabled(payload[0] == '1');
  }


  if (command.equals("channel")) {
    payload[length] = 0;
    String msg = (String)(char*)payload;
    if (msg.equals("")) {
      LogTarget.println((String)"set channel to default (0)");
      textcycle[0] = 0;
      num_textcycles = 1;
      current_channel = textcycle[current_cycle];
    } else {
      // parse comma-separated list of channels (or a single channel number)
      LogTarget.println((String)"set channel to comma-separated list: " + msg);
      num_textcycles = 0;
      int pos = 0;
      while (1) {
        int newpos = msg.substring(pos).indexOf(",");
        if (newpos != -1) {
          LogTarget.println((String)"pos=" + pos + " -> search [" + msg.substring(pos)
                            + "] => newpos=" + (pos + newpos)
                            + " => add substring " + msg.substring(pos, pos + newpos));
          textcycle[num_textcycles++] = msg.substring(pos, pos + newpos).toInt();
          if (num_textcycles == MAX_TEXTCYCLE) break; // no more room left in array
          pos = pos + newpos + 1;
        } else {
          LogTarget.println((String)"pos=" + pos + " -> search [" + msg.substring(pos)
                            + "] => not found"
                            + " => add substring " + msg.substring(pos));
          textcycle[num_textcycles++] = msg.substring(pos).toInt();
          break; // done!
        }
      }
      LogTarget.print((String)"Found " + num_textcycles + " cycle indices:");
      for (int i = 0; i < num_textcycles; i++)
        LogTarget.print((String)" " + textcycle[i]);
      LogTarget.println(".");
      current_cycle = 0;
      current_channel = textcycle[current_cycle];
      current_cycle = (current_cycle + 1) % num_textcycles;
    }
    // if display is scrolling, text will be changed at the end of the current cycle
    if (scrollDelay == 0) {
      // otherwise trigger redraw on display
      textIndex = 0;
    }
    return;
  }


  if (command.startsWith("text")) {
    const bool pr = DEBUGPRINT;                   // set to 1 for debug prints
    if (pr) {
      LogTarget.print("content = [");
      printHex8(payload, length);
      LogTarget.println((String)"] (" + length + " bytes)");
    }

    int target_channel;
    if (command.equals("text"))
      target_channel = 0;
    else
      target_channel = command.substring(command.lastIndexOf("/") + 1).toInt();
    if (target_channel < 0) target_channel = 0;
    if (target_channel > NUM_CHANNELS - 1) target_channel = NUM_CHANNELS - 1;
    LogTarget.println((String)"Target text channel: " + target_channel);

    uint8_t* text = texts[target_channel];
    text[0] = ' ';
    for (int i = 0 ; i < MAX_TEXT_LENGTH; i++) {
      text[i] = 0;
    }
    char tmp[16];
    int i = 0, j = 0;
    while (i < length) {
      uint8_t b = payload[i++];
      if (pr) {
        sprintf(tmp, "0x%.2X = '%c' -> ", b, b);
        LogTarget.print(tmp);
      }
      if ((b & 0b10000000) == 0) {                    // 7-bit ASCII
        if (pr) LogTarget.println("ASCII");
        if (b == 0 || b == 10) {
          b = 32;
        } else if (b < 32) {
          LogTarget.println((String)"** Replace unknown control code <" + b + "> with checkerboard");
          b = 127;
        }
        text[j++] = b;

        /* extending the character set:
             1 - find a suitable position in the font table ( = code point) in file font.h
             2 - put the glyph width and its pixel pattern into the font table
             3 - find out the Unicode (UTF-8) representation of the glyph
                 e.g. by setting "const bool pr = 1;" in line 343 above
             4 - supplement the decoder in the following lines
               4.1 - two-byte sequence: find out if it starts with 0xC2, C3, C4, …
                     and check for the second byte
               4.2 - three-byte sequence: find out the prefix (0xE2)
                     and check for the second and third bytes
               4.3 - put the code point index into text[j++]
             5 - don't forget the else parts when introducing a new prefix
        */

      } else if ((b & 0b11100000) == 0b11000000) {    // UTF-8 2-byte sequence: starts with 0b110xxxxx
        if (pr) LogTarget.println("UTF-8 (2)");
        if (b == 0xC3) {                              // UTF-8 1st byte
          text[j++] = payload[i++] + 64;              // map 2nd byte to Latin-1 (simplified)

        } else if (b == 0xC2) {
          if (i < length && payload[i] == 0x86) {           //  = dagger sign
            text[j++] = 134;
          } else if (i < length && payload[i] == 0x87) {    //  = double dagger sign
            text[j++] = 135;
          } else if (i < length && payload[i] == 0x89) {    //  = per mille sign
            text[j++] = 137;
          } else if (i < length && payload[i] == 0xA0) {    // no-break space (wide space)
            text[j++] = 129;
          } else if (i < length && payload[i] == 0xA7) {    // § = section sign
            text[j++] = 167;
          } else if (i < length && payload[i] == 0xB0) {    // ° = degrees sign
            text[j++] = 176;
          } else if (i < length && payload[i] == 0xB1) {    // ± = plus/minus sign
            text[j++] = 177;
          } else if (i < length && payload[i] == 0xB2) {    // ² = superscript 2
            text[j++] = 178;
          } else if (i < length && payload[i] == 0xB3) {    // ³ = superscript 3
            text[j++] = 179;
          } else if (i < length && payload[i] == 0xB5) {    // µ = mu
            text[j++] = 181;
          } else if (i < length && payload[i] == 0xB9) {    // ¹ = superscript 1
            text[j++] = 185;
          } else if (i < length && payload[i] == 0xBA) {    // º = masculine numeral indicator ("numero")
            text[j++] = 186;
          } else {
            // unknown
            text[j++] = 127;                         // add checkerboard pattern
          }
          i += 1;
        } else if (b == 0xC4) {
          if (i < length && payload[i] == 0x8C) {           // Č = C with caron
            text[j++] = 142;
          } else if (i < length && payload[i] == 0x8D) {    // č = c with caron
            text[j++] = 157;
          } else {
            // unknown
            text[j++] = 127;                         // add checkerboard pattern
          }
        } else if (b == 0xC5) {
          if (i < length && payload[i] == 0xA0) {           // Š = S with caron
            text[j++] = 138;
          } else if (i < length && payload[i] == 0xA1) {    // š = s with caron
            text[j++] = 154;
          } else if (i < length && payload[i] == 0xBD) {    // Ž = Z with caron
            text[j++] = 143;
          } else if (i < length && payload[i] == 0xBE) {    // ž = z with caron
            text[j++] = 158;
          } else {
            // unknown
            text[j++] = 127;                         // add checkerboard pattern
          }
        } else {
          // unknown
          text[j++] = 127;                           // add checkerboard pattern
          i += 1;
        }

      } else if ((b & 0b11110000) == 0b11100000) {   // UTF-8 3-byte sequence: starts with 0b1110xxxx
        if (pr) LogTarget.println("UTF-8 (3)");
        if (i + 1 < length && b == 0xE2) {
          if (payload[i] == 0x82 && payload[i + 1] == 0xAC) {
            text[j++] = 128;                               // € = euro sign
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x93) {
            text[j++] = 150;                               // – = en dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x94) {
            text[j++] = 151;                               // — = em dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0xA2) {
            text[j++] = 183;                               // • = bullet
          } else if (payload[i] == 0x80 && payload[i + 1] == 0xA6) {
            text[j++] = 133;                               // … = ellipsis
          } else if (payload[i] == 0x80 && payload[i + 1] == 0xB0) {
            text[j++] = 137;                               // ‰ = per mille
          } else {
            // unknown
            text[j++] = 127;                         // add checkerboard pattern
          }
          i += 2;
        } else {
          // unknown, skip remaining 2 bytes
          i += 2;
          text[j++] = 127;                           // add checkerboard pattern
          text[j++] = 127;                           // add checkerboard pattern
        }

      } else if ((b & 0b11111000) == 0b11110000) {   // UTF-8 4-byte sequence_ starts with 0b11110xxx
        if (pr) LogTarget.println("UTF-8 (4)");
        if (i + 2 < length && b == 0xF0) {
          if (payload[i] == 0x9F && payload[i + 1] == 0x90 && payload[i + 2] == 0xB1) {
            text[j++] = 129;                               // cat emoji
          } else {
            // unknown
            text[j++] = 127;                         // add checkerboard pattern
          }
          i += 3;
        } else {
          // unknown, skip remaining 3 bytes
          i += 3;
          text[j++] = 127;                             // add checkerboard pattern
          text[j++] = 127;                             // add checkerboard pattern
          text[j++] = 127;                             // add checkerboard pattern
        }

      } else {                                        // unsupported (illegal) UTF-8 sequence
        if (pr) LogTarget.print("UTF-8 (n) ");
        while ((payload[i] & 0b10000000) && i < length) {
          if (pr) LogTarget.print("+");
          i++;                                        // skip non-ASCII characters
          text[j++] = 127;                           // add checkerboard pattern
        }
        if (pr) LogTarget.println();
      }
    }
    for (int i = 0; i < j; i++) {
      uint8_t asc = text[i] - 32;
      uint16_t idx = pgm_read_word(&(font_index[asc]));
      uint8_t w = pgm_read_byte(&(font[idx]));
      if (w == 0) {
        // character is NOT defined, replace with 0x7f = checkerboard pattern
        text[i] = 0x7F;
      }
    }
    if (pr) {
      LogTarget.print((String)"=> Text[channel " + target_channel + "] (" + j + " bytes): ");
      printHex8(text, j + 1);
      LogTarget.println();
    }

    /** do NOT start new text
      textIndex = 0;
      colIndex = 0;
      marqueeDelayTimestamp = 0;
      scrollWhitespace = 0;
      led.clear();
    **/
  }
}
