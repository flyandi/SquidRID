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

#ifndef _SQUID_CONST_
#define _SQUID_CONST_


#define VERSION 1008
#define CMD_BAUDRATE 115200

#define CURRENT_INTERVAL 500
#define PEST_INTERVAL 9500
#define EXTERNAL_INTERVAL 1000
#define AUTO_START_TIMEOUT 30000

const char* PREF_APP = "__SQUID__";
const char* PREF_VERSION_KEY = "_V";
const char* PREF_RUN_KEY = "_R";
const char* PREF_PARAM_KEY = "_P";
const char* PREF_PATH_KEY = "_H";

typedef enum {
  MODE_SIM = 0,
  MODE_PEST = 1,
  MODE_EXTERNAL = 2,
} squid_app_mode_e;

typedef enum {
  EXTERNAL_NONE = 0,
  EXTERNAL_GPS = 1,
  EXTERNAL_LTM = 2,
} squid_external_mode_e;

typedef enum {
  SHIFT_NONE = 0,
  SHIFT_LOCATION = 1,
  SHIFT_RADIUS = 2,
} squid_shift_mode_e;

#endif
