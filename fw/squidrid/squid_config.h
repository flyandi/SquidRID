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
#ifndef _SQUID_CONFIG_
#define _SQUID_CONFIG_


///  ////////////////////////////////////////////////////////////////////////////////////////// ///
///  
///  This is the default configuration. If you never use the configurator you can
///  statically configure SquidRID here
///  
///  ////////////////////////////////////////////////////////////////////////////////////////// ///


#define USE_WIFI_NAN 0
#define USE_WIFI_BEACON 0
#define USE_WIFI 0  // set to 0 if any of above enabled
#define USE_BT 1    // ASTM F3411-19 /  ASD-STAN 4709-002.  .
#define USE_BEACON_FUNC 0
#define USE_NATIVE_WIFI 0

#define SATS_LEVEL_1 4
#define SATS_LEVEL_2 7
#define SATS_LEVEL_3 10

#define DEFAULT_ALT 137.0
#define DEFAULT_SPEED 60
#define DEFAULT_NAME "UAS_NO_NAME"
#define DEFAULT_REMOTE_ID ""
#define DEFAULT_DESCRIPTION "Recreational"




#endif

