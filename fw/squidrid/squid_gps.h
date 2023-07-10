/**
  _____  ___   __ __  ____  ___    ____   ____  ___
 / ___/ /   \ |  |  ||    ||   \  |    \ |    ||   \
(   \_ |     ||  |  | |  | |    \ |  D  ) |  | |    \
 \__  ||  Q  ||  |  | |  | |  D  ||    /  |  | |  D  |
 /  \ ||     ||  :  | |  | |     ||    \  |  | |     |
 \    ||     ||     | |  | |     ||  .  \ |  | |     |
  \___| \__,_| \__,_||____||_____||__|\_||____||_____|

 *
 * This file is part of SquidRID (https://github.com/flyandi/squidrid)
 *
 * Copyright (c) 2023 FLY&I (flyandi.net)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#ifndef _SQUID_GPS_
#define _SQUID_GPS_

#include <SoftwareSerial.h>

static struct
{
  float lat;
  float lng;
  int16_t alt;
  int16_t spd;
  uint8_t fix;
  uint8_t sats;
} GPS_DATA;

static SoftwareSerial gps_serial;

static float gps_parse_value(char *token, float d = 100.0) {
  return atof(token) / d;
}

static void gps_begin(uint16_t baud, uint8_t rx_pin, uint8_t tx_pin) {
  pinMode(rx_pin, INPUT);
  pinMode(tx_pin, OUTPUT);
  gps_serial.begin(baud, SWSERIAL_8N1, rx_pin, tx_pin, false);
}

static void gps_end() {
  gps_serial.end();
}

static void gps_parse(char c) {
  static char sentence[100];
  static int sentenceIndex = 0;

  if (c == '$') {
    // Start of a new NMEA sentence
    sentenceIndex = 0;
  } else if (c == '\n') {
    // End of the NMEA sentence
    sentence[sentenceIndex] = '\0';

    // Parse the sentence
    if (strstr(sentence, "GPGGA")) {
      char *token = strtok(sentence, ",");
      int tokenIndex = 0;

      //$GPGGA,212119.00,3332.62862,N,11738.69912,W,1,07,1.30,135.3,M,-33.0,M,,*6A
      while (token != NULL) {
        if (tokenIndex == 2) {
          // Latitude
          GPS_DATA.lat = gps_parse_value(token);
        } else if (tokenIndex == 3) {
          // Latitude hemisphere
          if (token[0] == 'S') {
            GPS_DATA.lat = -GPS_DATA.lat;
          }
        } else if (tokenIndex == 4) {
          // Longitude
          GPS_DATA.lng = gps_parse_value(token);
        } else if (tokenIndex == 5) {
          // Longitude hemisphere
          if (token[0] == 'W') {
            GPS_DATA.lng = -GPS_DATA.lng;
          }
        } else if (tokenIndex == 6) {
          // Fix status (0: Invalid, 1: GPS fix, 2: Differential GPS fix)
          GPS_DATA.fix = atoi(token);
        } else if (tokenIndex == 7) {
          // Number of satellites
          GPS_DATA.sats = atoi(token);
        } else if (tokenIndex == 9) {
          // Altitude
          GPS_DATA.alt = atof(token);
        } else if (tokenIndex == 11) {
          // Speed over ground
          GPS_DATA.spd = atof(token);
        }

        token = strtok(NULL, ",");
        tokenIndex++;
      }
    }
  } else {
    sentence[sentenceIndex++] = c;
  }
}

static void gps_loop() {
  if (gps_serial.available()) {
    char c = gps_serial.read();
    gps_parse(c);
  }
}

#endif  // eof