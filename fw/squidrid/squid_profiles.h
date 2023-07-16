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
#ifndef SQUID_PROFILES_H
#define SQUID_PROFILES_H

#define NAME_SIZE 24
#define SERIAL_SIZE 9

typedef struct {
  char name[NAME_SIZE];
  char min[NAME_SIZE];
  char max[NAME_SIZE];
} squid_profile_t;

// https://uasdoc.faa.gov/listDocs
squid_profile_t Squid_Profiles[] = {
  { "uAvionix pingRID", "1792C000000000000", "1792C000000FFFFFF" },
  { "Drone Tag", "1596F350000000000000", "1596F35FFFFFFFFFFFFF" },
  { "Camflite Aurora", "1871C000000007000", "1871C000000008999" },
  { "DJI Mini 3 Pro", "1581F4XF000000000000", "1581F4XFFFFFFFFFFFFF" },
  { "DJI Mavic 3", "1581F45T000000000000", "581F45TFFFFFFFFFFFF" },
  { "DJI Mavic 3 Cine", "1581F4QZ000000000000", "1581F4QZFFFFFFFFFFFF" },
  { "DJI Mavic 3 Cine", "1581F4QZ000000000000", "1581F4QZFFFFFFFFFFFF" },
  { "UAS RC", "1774FREM101000000001", "1774FREM101000075000" },
  { "WISPR Ranger Elite", "1889ERE000008012300", "1889ERE000012313099" }
};

const size_t squid_num_profiles = sizeof(Squid_Profiles) / sizeof(squid_profile_t);

const char* Squid_Descriptions[] = { "Recreational", "Commercial", "Industrial", "Military", "Remote Sensing", "n/a" };

const size_t squid_num_descriptions = sizeof(Squid_Descriptions) / sizeof(Squid_Descriptions[0]);

squid_profile_t getRandomProfile() {
  return Squid_Profiles[random(squid_num_profiles)];
}

void generateRandomSerialNumber(const char* minSerial, const char* maxSerial, char* randomSerial, int serialLength) {
  int minLength = strlen(minSerial);
  int maxLength = strlen(maxSerial);
  strncpy(randomSerial, minSerial, serialLength);
  for (int i = minLength; i < serialLength; i++) {
    randomSerial[i] = random(48, 58);  // Generate a random number character (ASCII range 48-57)
  }
  randomSerial[serialLength - 1] = '\0';
  if (strcmp(randomSerial, maxSerial) > 0) {
    strncpy(randomSerial, maxSerial, serialLength);
    randomSerial[serialLength - 1] = '\0';
  }
}

#endif
