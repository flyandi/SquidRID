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
#ifndef SQUID_INSTANCE_H
#define SQUID_INSTANCE_H

// CONFIGURATION ----------------------------------------------------------------------
#include "squid_config.h"

// INTERNAL ---------------------------------------------------------------------------
#define WIFI_CHANNEL 6  // Be careful changing this.
#define BEACON_FRAME_SIZE 512
#define BEACON_INTERVAL 0  // ms, defaults to 500. Android apps would prefer 100ms.
#define AUTH_DATUM 1546300800LU
#define PARAM_SIZE 24
#define PATH_SIZE 50  // 50 points
#define M_MPH_MS 0.44704

// INCLUDES ---------------------------------------------------------------------------
#include "opendroneid.h"
#include "squid_tools.h"
#include "squid_network.h"

// ENUM ----------------------------------------------------------------------------
typedef enum {
  SD_MODE_IDLE = 0,
  SD_MODE_FLY = 1,
} squid_mode_e;

typedef enum {
  SD_PATH_MODE_IDLE = 0,
  SD_PATH_MODE_RANDOM = 1,
  SD_PATH_MODE_FOLLOW = 2,
} squid_path_mode_e;

typedef enum {
  SD_PATH_TYPE_NONE = 0,
  SD_PATH_TYPE_GOTO = 1,    // uses heading and distance
  SD_PATH_TYPE_TRAVEL = 2,  // uses lat/lon in whatever heading
  SD_PATH_TYPE_SET = 3,     // sets lat/lon
} squid_path_type_e;

// STRUCTS ----------------------------------------------------------------------------
typedef struct {
  squid_path_type_e type;
  double param1;  // can be heading or lat
  double param2;  // can be distance (in m) or lon
} squid_path_t;

typedef struct {
  char uas_operator[PARAM_SIZE];
  char uas_description[PARAM_SIZE];
  char uas_id[PARAM_SIZE];
  ODID_uatype_t uas_type = ODID_UATYPE_NONE;
  ODID_idtype_t id_type = ODID_IDTYPE_NONE;
  char flight_desc[PARAM_SIZE];
  uint8_t
    region,
    spare1,
    eu_category,
    eu_class,
    id_type2,
    spare3;
  char id[PARAM_SIZE * 2];
  char secret[4];
  uint8_t spare[28];
} squid_params_t;

typedef struct {
  int years;
  int months;
  int days;
  int hours;
  int minutes;
  int seconds;
  int csecs;
  double latitude_d;
  double longitude_d;
  float alt_msl_m;
  float alt_agl_m;
  int speed;
  int heading;
  char *hdop_s;
  char *vdop_s;
  int satellites;
  double base_latitude;
  double base_longitude;
  double op_latitude;
  double op_longitude;
  float base_alt_m;
  float op_alt_m;
  int base_valid;
  int vel_N_cm;
  int vel_E_cm;
  int vel_D_cm;
} squid_data_t;

void construct2(void);
void init2(char *, int, uint8_t *, uint8_t);
int transmit_wifi2(uint8_t *, int);
int transmit_ble2(uint8_t *, int);

class Squid_Instance {

public:
  Squid_Instance();
  void update();
  void update(squid_params_t *);
  void setAuth(char *);
  void setAuth(uint8_t *, short int, uint8_t);
  int transmit();
  int transmit(squid_data_t *);

  void begin(Squid_Network *);
  void begin(Squid_Network *, squid_params_t);
  void loop(void);
  void setOriginLatLon(double lat, double lon);
  void setOperatorLatLon(double lat, double lon);
  void setAltitude(int a);
  void setOperatorAltitude(int a);
  void setName(const char *input);
  void setType(ODID_uatype_t type);
  void setRemoteId(const char *input, ODID_idtype_t type);
  void setSpeed(int speed);
  void setRemoteIdAsSerial(const char *input);
  void setRemoteIdAsFAARegistration(const char *input);
  void clearRemoteId();
  void setDescription(const char *input);
  void setMode(squid_mode_e m);
  void setPathMode(squid_path_mode_e m);
  void setDiffuser(uint32_t diff);

  void idlePath();
  void randomPath();
  void followPath(squid_path_t *, int size);
  void getParams(squid_params_t **);
  void getData(squid_data_t **);
  void getMac(uint8_t *);
  void setMac(uint8_t *);
  void setRandomMac();
  void reset();
  squid_mode_e getMode();
  squid_path_mode_e getPathMode();

private:
  void continueRandomPath();
  void continueFollowPath();

  Squid_Tools tools = {};
  squid_params_t params = {};
  squid_data_t data = {};
  squid_path_t path[PATH_SIZE];
  Stream *Debug_Serial = NULL;
  squid_mode_e mode = SD_MODE_IDLE;
  squid_path_mode_e pathMode = SD_PATH_MODE_IDLE;
  Squid_Network *network;

  LatLon_t
    path_origin,
    path_target;

  float
    x = 0.0,
    y = 0.0,
    z = 100.0,
    speed_m_x,
    max_dir_change = 75.0;

  double
    deg2rad = 0.0,
    m_deg_lat = 0.0,
    m_deg_long = 0.0;

  int
    phase = 0,
    path_mode = 0,
    path_size = 0,
    path_index = 0,
    auth_page = 0,
    auth_page_count = 0,
    key_length = 0,
    iv_length = 0;
  char
    wifi_ssid[32],
    *uas_operator;

  uint32_t
    last_update,
    last_ble,
    last_msecs = 2000,
    path_ms_t = 0;

  uint16_t
    alt = DEFAULT_ALT,
    wifi_interval = 0,
    ble_interval = 0;

  uint8_t
    wifi_mac[6],
    wifi_channel = WIFI_CHANNEL,
    *auth_key = NULL,
    *auth_iv = NULL,
    msg_counter[16],
    use_bt = 0,
    use_wifi = 0;

  size_t
    wifi_ssid_length = 0;

  int transmit_wifi(squid_data_t *, int);
  int transmit_ble(uint8_t *, int);

#if USE_WIFI
  uint16_t sequence = 1, beacon_interval = 0x200;
#if USE_WIFI_BEACON
  int beacon_offset = 0, beacon_max_packed = 30;
  uint8_t beacon_frame[BEACON_FRAME_SIZE],
#if USE_BEACON_FUNC
    beacon_counter = 0;
#else
    *beacon_payload, *beacon_timestamp, *beacon_counter, *beacon_length, *beacon_seq;
#endif
#endif
#endif

#if USE_WIFI_BEACON && (!USE_BEACON_FUNC)
  void init_beacon();
#endif

#if USE_BT
  uint8_t ble_message[36], counter = 0;
#endif

  ODID_UAS_Data UAS_data;
  ODID_BasicID_data *basicID_data;
  ODID_Location_data *location_data;
  ODID_Auth_data *auth_data[ODID_AUTH_MAX_PAGES];
  ODID_SelfID_data *selfID_data;
  ODID_System_data *system_data;
  ODID_OperatorID_data *operatorID_data;

  ODID_BasicID_encoded basicID_enc[2];
  ODID_Location_encoded location_enc;
  ODID_Auth_encoded auth_enc;
  ODID_SelfID_encoded selfID_enc;
  ODID_System_encoded system_enc;
  ODID_OperatorID_encoded operatorID_enc;
};

#endif
