/**
  _____  ___   __ __  ____  ___    ____   ____  ___
 / ___/ /   \ |  |  ||    ||   \  |    \ |    ||   \
(   \_ |     ||  |  | |  | |    \ |  D  ) |  | |    \
 \__  ||  Q  ||  |  | |  | |  D  ||    /  |  | |  D  |
 /  \ ||     ||  :  | |  | |     ||    \  |  | |     |
 \    ||     ||     | |  | |     ||  .  \ |  | |     |
  \___| \__,_| \__,_||____||_____||__|\_||____||_____|

 *
 * SquidRID (https://github.com/flyandi/squidrid)
 *
 * WARNING: THIS SOFTWARE IS FOR EDUCATIONAL PURPOSES ONLY
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

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

#include <Arduino.h>
#include <Preferences.h>
#include "squid_tools.h"
#include "squid_instance.h"
#include "squid_network.h"
#include "squid_def.h"
#include "squid_cmd.h"
#include "squid_profiles.h"
#include "squid_ltm.h"
#include "squid_gps.h"

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

static Squid_Network network;
static Squid_Instance squid;
static Squid_Tools tool;
static runtime_t RUNTIME = {};
static Preferences preferences;
static uint32_t current_t;
static uint32_t pest_t;
static uint32_t auto_t;
static bool in_serial = false;
cmd_action_e cmd_action;

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  init_cmd();
  init_squid();
  init_runtime();
  delay(2000);

  auto_t = millis();
}

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

void init_runtime() {
  RUNTIME.mode = MODE_SIM;
  recover();
  update_squid();
}

void init_squid() {
  tool.setupTime();
  network.begin(SD_NETWORK_MODE_BT);

  squid.begin(&network);
  squid.setMode(SD_MODE_IDLE);
  squid.setType(ODID_UATYPE_AEROPLANE);
  squid.getParams(&RUNTIME.params);
  RUNTIME.fly_mode = squid.getMode();

  /*squid_path_t follow[] = {
    { SD_PATH_TYPE_GOTO, 0, 100 },
    { SD_PATH_TYPE_GOTO, 90, 100 },
    { SD_PATH_TYPE_GOTO, 180, 100 },
    { SD_PATH_TYPE_GOTO, 270, 100 },
  };*/
}

void update_squid() {

  if (RUNTIME.mode == MODE_PEST) {
    squid_profile_t profile = getRandomProfile();
    char serial[24];

    LatLon_t c;
    tool.generateRandomPointInCircle(RUNTIME.pe_lat, RUNTIME.pe_lng, RUNTIME.pe_radius, &c);
    generateRandomSerialNumber(profile.min, profile.max, serial, 24);

    squid.reset();
    squid.setName(profile.name);
    squid.setDescription(Squid_Descriptions[random(squid_num_descriptions)]);
    squid.setRemoteId(serial, ODID_IDTYPE_SERIAL_NUMBER);
    squid.setType(ODID_UATYPE_HELICOPTER_OR_MULTIROTOR);
    squid.setPathMode(SD_PATH_MODE_RANDOM);
    squid.setOriginLatLon(c.lat, c.lon);
    squid.setAltitude(random(1, 25) * 25);
    squid.setOperatorLatLon(c.lat, c.lon);
    squid.setOperatorAltitude(-1000);
    squid.setSpeed(random(1, 30) * 10);
    squid.setRandomMac();
  }

  if (RUNTIME.mode == MODE_SIM) {
    bool isEmpty = true;
    for (int i = 0; i < 6; i++) {
      if (RUNTIME.mac[i] != 0x00) {
        isEmpty = false;
        break;
      }
    }
    if (!isEmpty) {
      squid.setMac(RUNTIME.mac);
    }

    squid.setPathMode(RUNTIME.path_mode);
    squid.setOriginLatLon(RUNTIME.lat, RUNTIME.lng);
    squid.setAltitude(RUNTIME.alt);
    squid.setOperatorLatLon(RUNTIME.op_lat, RUNTIME.op_lng);
    squid.setOperatorAltitude(RUNTIME.op_alt);
    squid.setSpeed(RUNTIME.speed);
  }

  squid.setMode(RUNTIME.fly_mode);
  squid.update();
  squid.getData(&RUNTIME.data);
  squid.getMac(RUNTIME.mac);
}

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

void loop() {

  if (RUNTIME.fly_mode == SD_MODE_IDLE && !in_serial) {
    if (millis() - auto_t > AUTO_START_TIMEOUT) {
      in_serial = true;
      RUNTIME.fly_mode == SD_MODE_FLY;
      update_squid();
    }
  }

  if (RUNTIME.mode == MODE_PEST) {
    if (millis() - pest_t > (RUNTIME.pe_spawn * 1000)) {
      if (RUNTIME.fly_mode == SD_MODE_FLY) {
        update_squid();
      }
      pest_t = millis();
    }
  }

  loop_cmd();
  squid.loop();
  network.loop();

  if (millis() - current_t > CURRENT_INTERVAL) {
    if (squid.getMode() == SD_MODE_FLY) {
      _cmd_current(&RUNTIME);
    }
    current_t = millis();
  }
}


void loop_cmd() {
  cmd_action = process_cmd(&RUNTIME);
  if (cmd_action == CMD_STORE) {
    store();
    update_squid();
    Serial.println("$%");
  }

  if (cmd_action == CMD_STORE || cmd_action == CMD_INFO) {
    in_serial = true;
  }
}

///  ////////////////////////////////////////////////////////////////////////////////////////// ///

void store() {
  Preferences preferences;
  squid_mode_t fly_mode = RUNTIME.fly_mode;
  RUNTIME.fly_mode = SD_MODE_IDLE;
  preferences.begin(PREF_APP, false);
  preferences.putInt(PREF_VERSION_KEY, VERSION);
  preferences.putBytes(PREF_RUN_KEY, &RUNTIME, sizeof(runtime_t));
  preferences.putBytes(PREF_PARAM_KEY, RUNTIME.params, sizeof(squid_params_t));
  preferences.putBytes(PREF_PATH_KEY, &RUNTIME.path, sizeof(RUNTIME.path));
  preferences.end();
  RUNTIME.fly_mode = fly_mode;
}

void recover() {
  Preferences preferences;
  preferences.begin(PREF_APP, true);
  if (preferences.getInt(PREF_VERSION_KEY) == VERSION) {
    size_t ds = preferences.getBytes(PREF_RUN_KEY, &RUNTIME, sizeof(runtime_t));
    if (ds == sizeof(runtime_t)) {
      preferences.getBytes(PREF_PARAM_KEY, RUNTIME.params, sizeof(squid_params_t));
      //preferences.getBytes(PREF_PATH_KEY, RUNTIME.path, sizeof(RUNTIME.path));
    }
  }
  preferences.end();
}
