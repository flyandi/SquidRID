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

#ifndef _SQUID_CMD_
#define _SQUID_CMD_

#include <vector>
#include <Arduino.h>
#include <cstring>
#include "squid_def.h"
#include "squid_const.h"
#include "squid_instance.h"

typedef enum {
  CMD_NONE = 0,
  CMD_STORE = 1,
  CMD_TOGGLE_STATE = 2,
  CMD_STORE_UPDATE = 3,
  CMD_INFO = 4,
} cmd_action_e;

char AttrDelimiter = '|';

struct Attr {
  String value;

  Attr(String val)
    : value(val) {}

  String asString() {
    return value;
  }

  float asFloat() {
    if (value.length() == 0) {
      return 0;
    }
    return value.toFloat();
  }

  int asInt() {
    if (value.length() == 0) {
      return 0;
    }
    return value.toInt();
  }
};


std::vector<Attr> _parseAttr(const String &input, char delimiter) {
  char buffer[input.length() + 1];
  input.toCharArray(buffer, input.length() + 1);
  std::vector<Attr> tokens;
  char *token = strtok(buffer, &delimiter);
  while (token != NULL) {
    tokens.push_back(Attr(String(token)));
    token = strtok(NULL, &delimiter);
  }

  return tokens;
}

typedef struct
{
  const char *name;
  cmd_action_e (*handler)(runtime_t *runtime, const String &value);
} cmd_command_t;


void _cmd_current(runtime_t *runtime) {

  if (runtime->mode == MODE_SIM) {
    Serial.printf("$C|%f|%f|%f|%f|%f|%f|%d|%d|%d|%d|%d\r\n",
                  runtime->data->latitude_d,
                  runtime->data->longitude_d,
                  runtime->data->op_latitude,
                  runtime->data->op_longitude,
                  runtime->data->base_alt_m,
                  runtime->data->op_alt_m,
                  runtime->data->speed,
                  runtime->data->heading,
                  runtime->data->satellites,
                  runtime->fly_mode,
                  runtime->path_mode);
  }

  if (runtime->mode == MODE_PEST) {
    char mac[18];
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", runtime->mac[0], runtime->mac[1], runtime->mac[2], runtime->mac[3], runtime->mac[4], runtime->mac[5]);
    Serial.printf("$T|%f|%f|%f|%d|%d|%s|%s|%s|%s|%d|%d|%d\r\n",
                  runtime->data->latitude_d,
                  runtime->data->longitude_d,
                  runtime->data->base_alt_m,
                  runtime->data->speed,
                  runtime->data->heading,
                  mac,
                  runtime->params->uas_id,
                  runtime->params->uas_operator,
                  runtime->params->uas_description,
                  runtime->params->uas_type,
                  runtime->params->id_type,
                  runtime->fly_mode);
  }
}

const cmd_command_t _cmd_commands[] = {
  // Reboot
  { "$R", [](runtime_t *runtime, const String &value) {
     ESP.restart();
     return CMD_NONE;
   } },
  // Version
  { "$V", [](runtime_t *runtime, const String &value) {
     Serial.printf("$V|%d\r\n", VERSION);
     return CMD_INFO;
   } },
  // Current Position
  { "$C", [](runtime_t *runtime, const String &value) {
     _cmd_current(runtime);
     return CMD_INFO;
   } },
  // Data
  { "$D", [](runtime_t *runtime, const String &value) {
     char mac[18];
     sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", runtime->mac[0], runtime->mac[1], runtime->mac[2], runtime->mac[3], runtime->mac[4], runtime->mac[5]);

     uint8_t length = 0;
     for (uint8_t i = 0; i < MAX_SQUID_PATH; i++) {
       if (runtime->path[i].type == SD_PATH_TYPE_NONE) {
         break;
       }
       length = i + 1;
     }

     size_t pathLength = length * 20 + length - 1;
     char *path = (char *)malloc(pathLength * sizeof(char));
     if (path != NULL) {
       size_t offset = 0;
       for (size_t i = 0; i < length; i++) {
         offset += snprintf(path + offset, pathLength - offset, "%g|%g|", runtime->path[i].param1, runtime->path[i].param2);
       }
     }

     Serial.printf("$D|%d|%s|%s|%s|%d|%d|%f|%f|%d|%f|%f|%d|%d|%d|%s|%d|%f|%f|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%s\r\n",
                   VERSION,
                   runtime->params->uas_id,
                   runtime->params->uas_operator,
                   runtime->params->uas_description,
                   runtime->params->uas_type,
                   runtime->params->id_type,
                   runtime->lat,
                   runtime->lng,
                   runtime->alt,
                   runtime->op_lat,
                   runtime->op_lng,
                   runtime->op_alt,
                   runtime->speed,
                   runtime->sats,
                   mac,
                   runtime->mode,
                   runtime->pe_lat,
                   runtime->pe_lng,
                   runtime->pe_radius,
                   runtime->pe_spawn,
                   runtime->ext_protocol,
                   runtime->ext_baud,
                   runtime->ext_rx_pin,
                   runtime->ext_tx_pin,
                   runtime->ext_shift_mode,
                   runtime->ext_shift_radius,
                   runtime->ext_shift_min,
                   runtime->ext_shift_max,
                   length,
                   length > 0 ? path : "");
     return CMD_INFO;
   } },

  // Store Data
  { "$SD", [](runtime_t *runtime, const String &value) {
     std::vector<Attr> tokens = _parseAttr(value, AttrDelimiter);
     if (tokens.size() >= 19) {
       strlcpy(runtime->params->uas_id, tokens[0].asString().c_str(), sizeof(runtime->params->uas_id));
       strlcpy(runtime->params->uas_operator, tokens[1].asString().c_str(), sizeof(runtime->params->uas_operator));
       strlcpy(runtime->params->uas_description, tokens[2].asString().c_str(), sizeof(runtime->params->uas_description));
       runtime->params->uas_type = ODID_uatype_t(tokens[3].asInt());
       runtime->params->id_type = ODID_idtype_t(tokens[4].asInt());
       runtime->lat = tokens[5].asFloat();
       runtime->lng = tokens[6].asFloat();
       runtime->alt = tokens[7].asInt();
       runtime->op_lat = tokens[8].asFloat();
       runtime->op_lng = tokens[9].asFloat();
       runtime->op_alt = tokens[10].asInt();
       runtime->speed = tokens[11].asInt();
       runtime->sats = tokens[12].asInt();
       sscanf(tokens[13].asString().c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &runtime->mac[0], &runtime->mac[1], &runtime->mac[2], &runtime->mac[3], &runtime->mac[4], &runtime->mac[5]);
       runtime->mode = squid_app_mode_e(tokens[14].asInt());
       runtime->pe_lat = tokens[15].asFloat();
       runtime->pe_lng = tokens[16].asFloat();
       runtime->pe_radius = tokens[17].asInt();
       runtime->pe_spawn = tokens[18].asInt();
       if (tokens.size() == 27) {
         runtime->ext_protocol = squid_external_mode_e(tokens[19].asInt());
         runtime->ext_baud = tokens[20].asInt();
         runtime->ext_rx_pin = tokens[21].asInt();
         runtime->ext_tx_pin = tokens[22].asInt();
         runtime->ext_shift_mode = squid_shift_mode_e(tokens[23].asInt());
         runtime->ext_shift_radius = tokens[24].asInt();
         runtime->ext_shift_min = tokens[25].asInt();
         runtime->ext_shift_max = tokens[26].asInt();
       }
       return CMD_STORE;
     }
     return CMD_NONE;
   } },

  // Store Modes and Paths
  { "$SM", [](runtime_t *runtime, const String &value) {
     //Serial.println(value);
     std::vector<Attr> tokens = _parseAttr(value, AttrDelimiter);
     if (tokens.size() >= 6) {
       runtime->mode = squid_app_mode_e(tokens[0].asInt());
       runtime->fly_mode = squid_mode_e(tokens[1].asInt());
       runtime->path_mode = squid_path_mode_e(tokens[2].asInt());
       runtime->speed = tokens[3].asInt();
       runtime->alt = tokens[4].asInt();

       std::memset(runtime->path, SD_PATH_TYPE_NONE, sizeof(runtime->path));

       // $SM|0|0|2|0|0|6|179|213|91|320|10|250|342|231|271|386|161|270
       uint8_t pc = (tokens.size() - 6) / 2;
       if (pc > 0 && pc <= MAX_SQUID_PATH) {
         for (uint8_t i = 0, n = 0; i < pc; i++, n += 2) {
           runtime->path[i] = { SD_PATH_TYPE_GOTO, static_cast<double>(tokens[6 + n].asFloat()), static_cast<double>(tokens[6 + n + 1].asFloat()) };
         }
       }

       return CMD_STORE;
     }
     return CMD_NONE;
   } }

};

const int _cmd_num_commands = sizeof(_cmd_commands) / sizeof(_cmd_commands[0]);

void init_cmd() {
  Serial.begin(CMD_BAUDRATE);
}

cmd_action_e process_cmd(runtime_t *runtime) {
  cmd_action_e action = CMD_NONE;
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    bool found = false;
    for (int i = 0; i < _cmd_num_commands; i++) {
      if (command.startsWith(_cmd_commands[i].name)) {
        String cmd = String(_cmd_commands[i].name);
        String value = command.substring(cmd.length());
        value.trim();
        action = _cmd_commands[i].handler(runtime, value);
        found = true;
        break;
      }
    }
    if (!found) {
      Serial.println(F("$-"));
    }
  }
  return action;
}

#endif
