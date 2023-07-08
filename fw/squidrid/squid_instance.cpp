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
#define DIAGNOSTICS 0
#pragma GCC diagnostic warning "-Wunused-variable"

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

extern "C" {
  int clock_gettime(clockid_t, struct timespec *);
  uint64_t alt_unix_secs(int, int, int, int, int, int);
}

#include "squid_instance.h"

Squid_Instance::Squid_Instance() {
  deg2rad = (4.0 * atan(1.0)) / 180.0;
  memset(&params, 0, sizeof(squid_params_t));
  memset(&data, 0, sizeof(squid_data_t));
  memset(wifi_mac, 0, 6);
  memset(wifi_ssid, 0, sizeof(wifi_ssid));
  memset(msg_counter, 0, sizeof(msg_counter));

  strcpy(wifi_ssid, "UAS_ID_OPEN");
  uas_operator = "";

  setRandomMac();

#if USE_WIFI
  beacon_interval = (BEACON_INTERVAL) ? BEACON_INTERVAL : 500;
#if USE_WIFI_BEACON
  memset(beacon_frame, 0, BEACON_FRAME_SIZE);
#if !USE_BEACON_FUNC
  beacon_counter =
    beacon_length =
      beacon_timestamp =
        beacon_seq =
          beacon_payload = beacon_frame;

#endif
#endif
#endif

  int i;
  memset(&UAS_data, 0, sizeof(ODID_UAS_Data));

  basicID_data = &UAS_data.BasicID[0];
  location_data = &UAS_data.Location;
  selfID_data = &UAS_data.SelfID;
  system_data = &UAS_data.System;
  operatorID_data = &UAS_data.OperatorID;

  for (i = 0; i < ODID_AUTH_MAX_PAGES; ++i) {

    auth_data[i] = &UAS_data.Auth[i];

    auth_data[i]->DataPage = i;
    auth_data[i]->AuthType = ODID_AUTH_NONE;  // 0
  }

  UAS_data.BasicID[0].IDType = ODID_IDTYPE_NONE;  // 0
  UAS_data.BasicID[0].UAType = ODID_UATYPE_NONE;  // 0
  UAS_data.BasicID[1].IDType = ODID_IDTYPE_NONE;  // 0
  UAS_data.BasicID[1].UAType = ODID_UATYPE_NONE;  // 0

  odid_initLocationData(location_data);

  location_data->Status = ODID_STATUS_UNDECLARED;  // 0
  location_data->SpeedVertical = INV_SPEED_V;
  location_data->HeightType = ODID_HEIGHT_REF_OVER_TAKEOFF;
  location_data->HorizAccuracy = ODID_HOR_ACC_10_METER;
  location_data->VertAccuracy = ODID_VER_ACC_10_METER;
  location_data->BaroAccuracy = ODID_VER_ACC_10_METER;
  location_data->SpeedAccuracy = ODID_SPEED_ACC_10_METERS_PER_SECOND;
  location_data->TSAccuracy = ODID_TIME_ACC_1_5_SECOND;

  selfID_data->DescType = ODID_DESC_TYPE_TEXT;
  strcpy(selfID_data->Desc, DEFAULT_DESCRIPTION);

  odid_initSystemData(system_data);

  system_data->OperatorLocationType = ODID_OPERATOR_LOCATION_TYPE_TAKEOFF;
  system_data->ClassificationType = ODID_CLASSIFICATION_TYPE_UNDECLARED;
  system_data->AreaCount = 1;
  system_data->AreaRadius = 500;
  system_data->AreaCeiling =
    system_data->AreaFloor = -1000.0;
  system_data->CategoryEU = ODID_CATEGORY_EU_UNDECLARED;
  system_data->ClassEU = ODID_CLASS_EU_UNDECLARED;
  system_data->OperatorAltitudeGeo = -1000.0;

  operatorID_data->OperatorIdType = ODID_OPERATOR_ID;

#if DIAGNOSTICS
  Debug_Serial = &Serial;
#endif

  return;
}

void Squid_Instance::begin(Squid_Network *n, squid_params_t p) {
  network = n;
  params = p;
  setAltitude(alt);
  data.satellites = 8;
  data.base_valid = 1;
}

void Squid_Instance::begin(Squid_Network *n) {
  squid_params_t p;
  p.region = 0;
  p.eu_category = 0;
  p.eu_class = 0;
  p.uas_type = ODID_UATYPE_NONE;
  begin(n, p);
  setSpeed(DEFAULT_SPEED);
  setName(DEFAULT_NAME);
  setRemoteId(DEFAULT_REMOTE_ID, ODID_IDTYPE_SERIAL_NUMBER);
  setDescription(DEFAULT_DESCRIPTION);
  setType(ODID_UATYPE_NONE);
}

void Squid_Instance::loop() {
  bool isTransmit = true;
  uint32_t msecs;
  msecs = millis();

  if ((msecs - last_update) > 199) {

    last_update = msecs;

    if (mode == SD_MODE_FLY) {
      if (pathMode != SD_PATH_MODE_IDLE) {
        if (pathMode == SD_PATH_MODE_FOLLOW) {
          continueFollowPath();
        } else if (pathMode == SD_PATH_MODE_RANDOM) {
          continueRandomPath();
        }
      }
    }

    if (isTransmit) {
      transmit();
    }
  }
}

void Squid_Instance::continueFollowPath() {
  if (path_size == 0) {
    return;
  }

  bool next = false;

  if (path[path_index].type == SD_PATH_TYPE_SET) {
    data.latitude_d = path[path_index].param1;
    data.longitude_d = path[path_index].param2;
    next = true;
  } else if (path[path_index].type == SD_PATH_TYPE_GOTO) {

    // setup initial for next goto
    if (path_mode == 0) {
      path_ms_t = millis();
      data.heading = ((int)path[path_index].param1 + 360) % 360;
      path_mode = 1;
    }

    LatLon_t l;
    unsigned long ms = millis() - path_ms_t;
    bool d = tools.haversineAt(path_origin, data.heading, (double)speed_m_x, (int)path[path_index].param2, ms, &l);

    data.latitude_d = l.lat;
    data.longitude_d = l.lon;

    if (d) {
      path_origin.lat = data.latitude_d;
      path_origin.lon = data.longitude_d;
      path_mode = 0;
      next = true;
    }
  }

  if (next) {
    path_index++;
    if (path_index >= path_size) {
      path_index = 0;
    }
  }
}

void Squid_Instance::continueRandomPath() {
  int dir_change;
  float rads, ran;

  ran = 0.001 * (float)(((int)rand() % 1000) - 500);
  dir_change = (int)(max_dir_change * ran);
  data.heading = (data.heading + dir_change + 360) % 360;

  x += speed_m_x * sin(rads = (deg2rad * (float)data.heading));
  y += speed_m_x * cos(rads);

  data.latitude_d = data.base_latitude + (y / m_deg_lat);
  data.longitude_d = data.base_longitude + (x / m_deg_long);
}

void Squid_Instance::setOriginLatLon(double lat, double lon) {
  path_origin.lat = data.op_latitude = data.base_latitude = data.latitude_d = lat;
  path_origin.lon = data.op_longitude = data.base_longitude = data.longitude_d = lon;
  tools.calc_m_per_deg(lat, &m_deg_lat, &m_deg_long);
  path_index = 0;  // reset path, looks weird but whatever
}

void Squid_Instance::setOperatorLatLon(double lat, double lon) {
  data.op_latitude = lat;
  data.op_longitude = lon;
}

void Squid_Instance::setAltitude(int alt) {
  data.base_alt_m = alt;
  data.alt_msl_m = data.base_alt_m + z;
  data.alt_agl_m = z;
}

void Squid_Instance::setOperatorAltitude(int alt) {
  data.op_alt_m = alt;
}

void Squid_Instance::setName(char *input) {
  strcpy(params.uas_operator, input);
}

void Squid_Instance::setType(ODID_uatype_t type) {
  params.uas_type = type;
}

void Squid_Instance::setSpeed(int speed) {
  data.speed = speed;
  speed_m_x = ((float)speed) * M_MPH_MS / 5.0;
}

void Squid_Instance::setRemoteId(char *input, ODID_idtype_t type) {
  strcpy(params.uas_id, input);
  params.id_type = type;
}

void Squid_Instance::setRemoteIdAsSerial(char *input) {
  setRemoteId(input, ODID_IDTYPE_SERIAL_NUMBER);
}

void Squid_Instance::setRemoteIdAsFAARegistration(char *input) {
  setRemoteId(input, ODID_IDTYPE_ID_ASSIGNED_UUID);
}

void Squid_Instance::clearRemoteId() {
  setRemoteId("", ODID_IDTYPE_NONE);
}

void Squid_Instance::setDescription(char *input) {
  strcpy(params.uas_description, input);
}

void Squid_Instance::setMode(squid_mode_t m) {
  mode = m;
}

squid_mode_t Squid_Instance::getMode() {
  return mode;
}

void Squid_Instance::setPathMode(squid_path_mode_t m) {
  pathMode = m;
}

squid_path_mode_t Squid_Instance::getPathMode() {
  return pathMode;
}

void Squid_Instance::setDiffuser(uint32_t diff) {
  last_msecs += diff;
}

void Squid_Instance::getParams(squid_params_t **out) {
  *out = &params;
}

void Squid_Instance::getData(squid_data_t **out) {
  *out = &data;
}

void Squid_Instance::getMac(uint8_t *out) {
  memcpy(out, wifi_mac, sizeof(wifi_mac));
}

void Squid_Instance::setMac(uint8_t *in) {
  memcpy(wifi_mac, in, sizeof(wifi_mac));
}

void Squid_Instance::setRandomMac() {
  uint8_t mac[6];
  tools.generateMAC(mac);
  memcpy(wifi_mac, mac, sizeof(wifi_mac));
}

void Squid_Instance::idlePath() {
  pathMode = SD_PATH_MODE_IDLE;
}

void Squid_Instance::randomPath() {
  pathMode = SD_PATH_MODE_RANDOM;
}

void Squid_Instance::followPath(squid_path_t *p, int size) {
  if (size > PATH_SIZE) {
    size = PATH_SIZE;
  }
  path_size = size;
  path_index = 0;
  for (int i = 0; i < path_size; i++) {
    path[i] = p[i];
  }

  path_mode = 0;
  path_origin.lat = data.base_latitude;
  path_origin.lon = data.base_longitude;
  pathMode = SD_PATH_MODE_FOLLOW;
}

void Squid_Instance::reset() {
  last_msecs = 0;
  phase = 0;
}

void Squid_Instance::update() {
  update(&params);
}

void Squid_Instance::update(squid_params_t *parameters) {
  int status, i;
  char text[128];

  status = 0;
  text[0] = text[63] = 0;

  // operator
  uas_operator = parameters->uas_operator;
  strncpy(operatorID_data->OperatorId, parameters->uas_operator, ODID_ID_SIZE);
  operatorID_data->OperatorId[sizeof(operatorID_data->OperatorId) - 1] = 0;
  strcpy(selfID_data->Desc, params.uas_description);

  // basic
  UAS_data.BasicID[0].UAType = (ODID_uatype_t)parameters->uas_type;
  UAS_data.BasicID[1].UAType = (ODID_uatype_t)parameters->uas_type;
  UAS_data.BasicID[0].IDType = (ODID_idtype_t)parameters->id_type;
  UAS_data.BasicID[1].IDType = (ODID_idtype_t)parameters->id_type2;

  switch (basicID_data->IDType) {

    case ODID_IDTYPE_SERIAL_NUMBER:
    case ODID_IDTYPE_ID_ASSIGNED_UUID:
      strncpy(basicID_data->UASID, parameters->uas_id, ODID_ID_SIZE);
      break;

    case ODID_IDTYPE_CAA_REGISTRATION_ID:

      strncpy(basicID_data->UASID, parameters->uas_operator, ODID_ID_SIZE);
      break;
  }

  basicID_data->UASID[sizeof(basicID_data->UASID) - 1] = 0;

  // system

  if (parameters->region < 2) {

    system_data->ClassificationType = (ODID_classification_type_t)parameters->region;
  }

  if (parameters->eu_category < 4) {

    system_data->CategoryEU = (ODID_category_EU_t)parameters->eu_category;
  }

  if (parameters->eu_class < 8) {

    system_data->ClassEU = (ODID_class_EU_t)parameters->eu_class;
  }

  //

  encodeBasicIDMessage(&basicID_enc[0], &UAS_data.BasicID[0]);
  encodeBasicIDMessage(&basicID_enc[1], &UAS_data.BasicID[1]);
  encodeLocationMessage(&location_enc, location_data);
  encodeAuthMessage(&auth_enc, auth_data[0]);
  encodeSelfIDMessage(&selfID_enc, selfID_data);
  encodeSystemMessage(&system_enc, system_data);
  encodeOperatorIDMessage(&operatorID_enc, operatorID_data);

  //

  if (uas_operator[0]) {

    strncpy(wifi_ssid, uas_operator, i = sizeof(wifi_ssid));
    wifi_ssid[i - 1] = 0;
  }

  wifi_ssid_length = strlen(wifi_ssid);

  // init2(wifi_ssid, wifi_ssid_length, wifi_mac, wifi_channel);

#if USE_WIFI

#if USE_WIFI_BEACON && !USE_BEACON_FUNC

  init_beacon();

  // payload
  beacon_payload = &beacon_frame[beacon_offset];
  beacon_offset += 7;

  *beacon_payload++ = 0xdd;
  beacon_length = beacon_payload++;

  *beacon_payload++ = 0xfa;
  *beacon_payload++ = 0x0b;
  *beacon_payload++ = 0xbc;

  *beacon_payload++ = 0x0d;
  beacon_counter = beacon_payload++;

  beacon_max_packed = BEACON_FRAME_SIZE - beacon_offset - 2;

  if (beacon_max_packed > (ODID_PACK_MAX_MESSAGES * ODID_MESSAGE_SIZE)) {

    beacon_max_packed = (ODID_PACK_MAX_MESSAGES * ODID_MESSAGE_SIZE);
  }

#endif

#endif

  return;
}

/*
 *  These authentication functions need reviewing to make sure that they
 *  comply with opendroneid release 1.0.
 */

void Squid_Instance::setAuth(char *auth) {
  setAuth((uint8_t *)auth, strlen(auth), 0x0a);

  return;
}

//

void Squid_Instance::setAuth(uint8_t *auth, short int len, uint8_t type) {
  int i, j;
  char text[160];
  uint8_t check[32];

  auth_page_count = 1;

  if (len > MAX_AUTH_LENGTH) {

    len = MAX_AUTH_LENGTH;
    auth[len] = 0;
  }

  auth_data[0]->AuthType = (ODID_authtype_t)type;

  for (i = 0; (i < 17) && (auth[i]); ++i) {

    check[i] =
      auth_data[0]->AuthData[i] = auth[i];
  }

  check[i] =
    auth_data[0]->AuthData[i] = 0;

  if (Debug_Serial) {

    sprintf(text, "Auth. Code \'%s\' (%d)\r\n", auth, len);
    Debug_Serial->print(text);

    sprintf(text, "Page 0 \'%s\'\r\n", check);
    Debug_Serial->print(text);
  }

  if (len > 16) {

    for (auth_page_count = 1; (auth_page_count < ODID_AUTH_MAX_PAGES) && (i < len); ++auth_page_count) {

      auth_data[auth_page_count]->AuthType = (ODID_authtype_t)type;

      for (j = 0; (j < 23) && (i < len); ++i, ++j) {

        check[j] =
          auth_data[auth_page_count]->AuthData[j] = auth[i];
      }

      if (j < 23) {

        auth_data[auth_page_count]->AuthData[j] = 0;
      }

      check[j] = 0;

      if (Debug_Serial) {

        sprintf(text, "Page %d \'%s\'\r\n", auth_page_count, check);
        Debug_Serial->print(text);
      }
    }

    len = i;
  }

  auth_data[0]->LastPageIndex = (auth_page_count) ? auth_page_count - 1 : 0;
  auth_data[0]->Length = len;

  time_t secs;
  time(&secs);
  auth_data[0]->Timestamp = (uint32_t)(secs - AUTH_DATUM);

  if (Debug_Serial) {

    sprintf(text, "%d pages\r\n", auth_page_count);
    Debug_Serial->print(text);
  }

  return;
}

/*
 *
 */

int Squid_Instance::transmit() {
  return transmit(&data);
}

int Squid_Instance::transmit(squid_data_t *data) {
  int i, status;
  char text[128];
  uint32_t msecs;
  time_t secs = 0;

  i = 0;
  text[0] = 0;
  msecs = millis();
  time(&secs);

  if ((!system_data->OperatorLatitude) && (data->base_valid)) {

    system_data->OperatorLatitude = data->op_latitude;
    system_data->OperatorLongitude = data->op_longitude;
    system_data->OperatorAltitudeGeo = data->op_alt_m;

    system_data->Timestamp = (uint32_t)(secs - AUTH_DATUM);

    encodeSystemMessage(&system_enc, system_data);
  }

  if ((msecs > last_msecs) && ((msecs - last_msecs) > 74)) {

    last_msecs += 75;

    switch (phase) {

      case 0:
      case 8:
      case 16:
      case 24:
      case 32:
      case 4:
      case 12:
      case 20:
      case 28:
      case 36:  // Every 300 ms.

        if (data->satellites >= SATS_LEVEL_2) {

          location_data->Status = ODID_STATUS_UNDECLARED;
          location_data->Direction = (float)data->heading;
          location_data->SpeedHorizontal = M_MPH_MS * (float)(data->speed + (random(2001) / 10000.0 - 0.1) * data->speed);
          location_data->SpeedVertical = INV_SPEED_V;
          location_data->Latitude = data->latitude_d;
          location_data->Longitude = data->longitude_d;
          location_data->Height = data->alt_agl_m + (random(2001) / 10000.0 - 0.1) * data->alt_agl_m;  // add some random noise to it
          location_data->AltitudeGeo = data->alt_msl_m;
          location_data->TimeStamp = (float)((data->minutes * 60) + data->seconds) + 0.01 * (float)data->csecs;
        } else {

          location_data->Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
        }

        if ((status = encodeLocationMessage(&location_enc, location_data)) == ODID_SUCCESS) {

          transmit_ble((uint8_t *)&location_enc, sizeof(location_enc));
        } else if (Debug_Serial) {

          sprintf(text, "Squid_Instance::%s, encodeLocationMessage returned %d\r\n",
                  __func__, status);
          Debug_Serial->print(text);
        }

        break;

      case 6:
      case 14:
      case 22:
      case 30:
      case 38:  // Every 600 ms.

        if (secs > AUTH_DATUM) {

          system_data->Timestamp = (uint32_t)(secs - AUTH_DATUM);
          encodeSystemMessage(&system_enc, system_data);
        }

        transmit_ble((uint8_t *)&system_enc, sizeof(system_enc));

        break;

      case 2:

        if (UAS_data.BasicID[0].IDType) {

          transmit_ble((uint8_t *)&basicID_enc[0], sizeof(ODID_BasicID_encoded));
        }

        break;

      case 10:

        if (UAS_data.BasicID[1].IDType) {

          transmit_ble((uint8_t *)&basicID_enc[1], sizeof(ODID_BasicID_encoded));
        }

        break;

      case 18:

        transmit_ble((uint8_t *)&selfID_enc, sizeof(selfID_enc));
        break;

      case 26:

        transmit_ble((uint8_t *)&operatorID_enc, sizeof(operatorID_enc));
        break;

      case 34:

        if (auth_page_count) {

          // Refresh the timestamp on page 0?

          encodeAuthMessage(&auth_enc, auth_data[auth_page]);

          transmit_ble((uint8_t *)&auth_enc, sizeof(auth_enc));

          if (++auth_page >= auth_page_count) {

            auth_page = 0;
          }
        }

        break;

      default:

        break;
    }

    if (++phase > 39) {

      phase = 0;
    }
  }

  //

#if USE_WIFI

  // Pack and transmit the WiFi data.

  static uint8_t wifi_toggle = 1;
  static uint32_t last_wifi = 0;

  if ((msecs - last_wifi) >= beacon_interval) {

    last_wifi = msecs;

    if (wifi_toggle ^= 1) {  // IDs and locations.

      UAS_data.LocationValid =
        UAS_data.SystemValid = 1;

      if (UAS_data.BasicID[0].UASID[0]) {

        UAS_data.BasicIDValid[0] = 1;
      }

      if (UAS_data.BasicID[1].UASID[0]) {

        UAS_data.BasicIDValid[1] = 1;
      }

      if (UAS_data.OperatorID.OperatorId[0]) {

        UAS_data.OperatorIDValid = 1;
      }

      status = transmit_wifi(data, 0);

      UAS_data.BasicIDValid[0] =
        UAS_data.BasicIDValid[1] =
          UAS_data.LocationValid =
            UAS_data.SystemValid =
              UAS_data.OperatorIDValid = 0;
    } else {

      UAS_data.SelfIDValid = 1;

      for (i = 0; (i < auth_page_count) && (i < ODID_AUTH_MAX_PAGES); ++i) {

        UAS_data.AuthValid[i] = 1;
      }

      status = transmit_wifi(data, 0);

      UAS_data.SelfIDValid = 0;

      for (i = 0; (i < auth_page_count) && (i < ODID_AUTH_MAX_PAGES); ++i) {

        UAS_data.AuthValid[i] = 0;
      }
    }
  }

#endif  // USE_WIFI

  return status;
}

/*
 *
 */

int Squid_Instance::transmit_wifi(squid_data_t *data, int prepacked) {

#if USE_WIFI
  int length = 0, wifi_status = 0;
  uint32_t msecs;
  uint64_t usecs = 0;
  static uint32_t last_wifi = 0;
  char text[128];

  text[0] = 0;

  //Serial.printf("$#|%s, %s, %s", data->)

  //

  if (++sequence > 0xffffff) {

    sequence = 1;
  }

  msecs = millis();
  wifi_interval = msecs - last_wifi;
  last_wifi = msecs;

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  usecs = (uint64_t)((double)ts.tv_sec * 1e6 + (double)ts.tv_nsec * 1e-3);

#if USE_WIFI_NAN

  uint8_t buffer[1024];
  static uint8_t send_counter = 0;

  if ((length = odid_wifi_build_nan_sync_beacon_frame((char *)wifi_mac,
                                                      buffer, sizeof(buffer)))
      > 0) {

    wifi_status = transmit_wifi2(buffer, length);
  }

  if ((Debug_Serial) && ((length < 0) || (wifi_status != 0))) {

    sprintf(text, "odid_wifi_build_nan_sync_beacon_frame() = %d, transmit_wifi2() = %d\r\n",
            length, (int)wifi_status);
    Debug_Serial->print(text);
  }

  if ((length = odid_wifi_build_message_pack_nan_action_frame(&UAS_data, (char *)wifi_mac,
                                                              ++send_counter,
                                                              buffer, sizeof(buffer)))
      > 0) {

    wifi_status = transmit_wifi2(buffer, length);
  }

  if (Debug_Serial) {

    if ((length < 0) || (wifi_status != 0)) {

      sprintf(text, "odid_wifi_build_message_pack_nan_action_frame() = %d, transmit_wifi2() = %d\r\n",
              length, (int)wifi_status);
      Debug_Serial->print(text);

#if DIAGNOSTICS
    } else {

      sprintf(text, "Squid_Instance::%s ... ", __func__);
      Debug_Serial->print(text);

      for (int i = 0; i < 32; ++i) {

        sprintf(text, "%02x ", buffer[16 + i]);
        Debug_Serial->print(text);
      }

      Debug_Serial->print(" ... \r\n");

#endif
    }
  }

#endif  // NAN

#if USE_WIFI_BEACON

#if USE_BEACON_FUNC

  if ((length = odid_wifi_build_message_pack_beacon_frame(&UAS_data, (char *)wifi_mac,
                                                          wifi_ssid, wifi_ssid_length,
                                                          beacon_interval, ++beacon_counter,
                                                          beacon_frame, BEACON_FRAME_SIZE))
      > 0) {

    wifi_status = transmit_wifi2(beacon_frame, length);
  }

#if DIAGNOSTICS && 1

  if (Debug_Serial) {

    sprintf(text, "Squid_Instance::%s * %02x ... ", __func__, beacon_frame[0]);
    Debug_Serial->print(text);

    for (int i = 0; i < 20; ++i) {

      sprintf(text, "%02x ", beacon_frame[22 + i]);
      Debug_Serial->print(text);
    }

    Debug_Serial->print(" ... *\r\n");
  }

#endif  // DIAG

#else

  int i, len2 = 0;

  ++*beacon_counter;

  for (i = 0; i < 8; ++i) {

    beacon_timestamp[i] = (usecs >> (i * 8)) & 0xff;
  }

#if 1
  beacon_seq[0] = (uint8_t)(sequence << 4);
  beacon_seq[1] = (uint8_t)(sequence >> 4);
#endif

  length = (prepacked > 0) ? prepacked : odid_message_build_pack(&UAS_data, beacon_payload, beacon_max_packed);

  if (length > 0) {

    *beacon_length = length + 5;

    wifi_status = transmit_wifi2(beacon_frame, len2 = beacon_offset + length);
  }

#if DIAGNOSTICS && 1

  if (Debug_Serial) {

    sprintf(text, "Squid_Instance::%s %d %d+%d=%d ",
            __func__, beacon_max_packed, beacon_offset, length, len2);
    Debug_Serial->print(text);

    sprintf(text, "* %02x ... ", beacon_frame[0]);
    Debug_Serial->print(text);

    for (int i = 0; i < 16; ++i) {

      if ((i == 3) || (i == 10)) {

        Debug_Serial->print("| ");
      }

      sprintf(text, "%02x ", beacon_frame[beacon_offset - 10 + i]);
      Debug_Serial->print(text);
    }

    sprintf(text, "... %02x (%2d,%4u,%4u)\r\n", beacon_frame[len2 - 1],
            wifi_status, wifi_interval, ble_interval);
    Debug_Serial->print(text);
  }

#endif  // DIAG

#endif  // FUNC

#endif  // BEACON

#endif  // WIFI

  return 0;
}

/*
 *
 */

int Squid_Instance::transmit_ble(uint8_t *odid_msg, int length) {
  uint32_t msecs;

  msecs = millis();
  ble_interval = msecs - last_ble;
  last_ble = msecs;

  int i, j, k, len, status;
  uint8_t *a;

  i = j = k = len = 0;
  a = ble_message;

  memset(ble_message, 0, sizeof(ble_message));
  ble_message[j++] = 0x1e;
  ble_message[j++] = 0x16;
  ble_message[j++] = 0xfa;  // ASTM
  ble_message[j++] = 0xff;  //
  ble_message[j++] = 0x0d;
  ble_message[j++] = ++msg_counter[odid_msg[0] >> 4];

  for (i = 0; (i < length) && (j < sizeof(ble_message)); ++i, ++j) {
    ble_message[j] = odid_msg[i];
  }

  network->addMessage(wifi_mac, wifi_ssid, wifi_ssid_length, ble_message, j);
  return 0;
}

#if USE_WIFI_BEACON && (!USE_BEACON_FUNC)

void Squid_Instance::init_beacon() {
  int i;

  struct __attribute__((__packed__)) beacon_header {

    uint8_t control[2];    //  0-1:  frame control
    uint8_t duration[2];   //  2-3:  duration
    uint8_t dest_addr[6];  //  4-9:  destination
    uint8_t src_addr[6];   // 10-15: source
    uint8_t bssid[6];      // 16-21: base station
    uint8_t seq[2];        // 22-23: sequence
    uint8_t timestamp[8];  // 24-31:
    uint8_t interval[2];   //
    uint8_t capability[2];
  } * header;

  header = (struct beacon_header *)beacon_frame;
  beacon_timestamp = header->timestamp;
  beacon_seq = header->seq;

  header->control[0] = 0x80;
  header->interval[0] = (uint8_t)beacon_interval;
  header->interval[1] = (uint8_t)(beacon_interval >> 8);

  uint8_t capa[2] = { 0x21, 0x04 };
  memcpy(header->capability, capa, 2);

  for (i = 0; i < 6; ++i) {

    header->dest_addr[i] = 0xff;
    header->src_addr[i] =
      header->bssid[i] = wifi_mac[i];
  }

  beacon_offset = sizeof(struct beacon_header);

  beacon_frame[beacon_offset++] = 0;
  beacon_frame[beacon_offset++] = wifi_ssid_length;

  for (i = 0; (i < 32) && (wifi_ssid[i]); ++i) {

    beacon_frame[beacon_offset++] = wifi_ssid[i];
  }

  // Supported rates
  beacon_frame[beacon_offset++] = 0x01;
  beacon_frame[beacon_offset++] = 0x08;
  beacon_frame[beacon_offset++] = 0x8b;  //  5.5
  beacon_frame[beacon_offset++] = 0x96;  // 11
  beacon_frame[beacon_offset++] = 0x82;  //  1
  beacon_frame[beacon_offset++] = 0x84;  //  2
  beacon_frame[beacon_offset++] = 0x0c;  //  6
  beacon_frame[beacon_offset++] = 0x18;  // 12
  beacon_frame[beacon_offset++] = 0x30;  // 24
  beacon_frame[beacon_offset++] = 0x60;  // 48

  // DS
  beacon_frame[beacon_offset++] = 0x03;
  beacon_frame[beacon_offset++] = 0x01;
  beacon_frame[beacon_offset++] = wifi_channel;

  // Traffic Indication Map
  beacon_frame[beacon_offset++] = 0x05;
  beacon_frame[beacon_offset++] = 0x04;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x02;
  beacon_frame[beacon_offset++] = 0x00;
  beacon_frame[beacon_offset++] = 0x00;

  // Country Information
  beacon_frame[beacon_offset++] = 0x07;
  beacon_frame[beacon_offset++] = 0x06;
  beacon_frame[beacon_offset++] = 'U';
  beacon_frame[beacon_offset++] = 'S';
  beacon_frame[beacon_offset++] = 0x20;
  beacon_frame[beacon_offset++] = 0x01;
  beacon_frame[beacon_offset++] = 0x0d;
  beacon_frame[beacon_offset++] = 0x14;

  // Ext Rates
  beacon_frame[beacon_offset++] = 0x32;
  beacon_frame[beacon_offset++] = 0x04;
  beacon_frame[beacon_offset++] = 0x6c;  // 54
  beacon_frame[beacon_offset++] = 0x12;  //  9
  beacon_frame[beacon_offset++] = 0x24;  // 18
  beacon_frame[beacon_offset++] = 0x48;  // 36

  return;
}

#endif