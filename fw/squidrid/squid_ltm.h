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

#ifndef _SQUID_LTM_
#define _SQUID_LTM_

#include <SoftwareSerial.h>


enum
{
    LTM_IDLE,
    LTM_HEADER_START1,
    LTM_HEADER_START2,
    LTM_HEADER_MSGTYPE,
    LTM_HEADER_DATA
};

#define LTM_LONGEST_FRAME_LENGTH 18
#define LTM_GFRAMELENGTH 18
#define LTM_AFRAMELENGTH 10
#define LTM_SFRAMELENGTH 11
#define LTM_OFRAMELENGTH 18
#define LTM_NFRAMELENGTH 10
#define LTM_XFRAMELENGTH 10

const char *LTM_FLIGHT_MODES[] = {
    "Manual",
    "Rate",
    "Angle",
    "Horizon",
    "Acro",
    "Stabilized1",
    "Stabilized2",
    "Stabilized3",
    "Altitude Hold",
    "GPS Hold",
    "Waypoints",
    "Head free",
    "Circle",
    "RTH",
    "Follow me",
    "Land",
    "Fly by wire A",
    "Fly by wire B",
    "Cruise",
    "Unknown"};

static struct
{
    int pitch;
    int roll;
    int heading;
    uint16_t voltage;
    byte rssi;
    bool armed;
    bool failsafe;
    byte flightmode;

    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint8_t groundSpeed;
    int16_t hdop;
    uint8_t gpsFix;
    uint8_t gpsSats;

    int32_t homeLatitude;
    int32_t homeLongitude;

    uint8_t sensorStatus;
} LTM_DATA;

static SoftwareSerial ltm_serial;
static uint8_t ltm_buffer[LTM_LONGEST_FRAME_LENGTH];
static uint8_t ltm_state = LTM_IDLE;
static char ltm_frameType;
static byte ltm_frameLength;
static byte ltm_receiverIndex;

byte ltm_readByte(uint8_t offset)
{
    return ltm_buffer[offset];
}

int ltm_readInt(uint8_t offset)
{
    return (int)ltm_buffer[offset] + ((int)ltm_buffer[offset + 1] << 8);
}

uint16_t ltm_readInt_u16(uint8_t offset)
{
    return (uint16_t)ltm_buffer[offset] + ((uint16_t)ltm_buffer[offset + 1] << 8);
}

int32_t ltm_readInt32(uint8_t offset)
{
    return (int32_t)ltm_buffer[offset] + ((int32_t)ltm_buffer[offset + 1] << 8) + ((int32_t)ltm_buffer[offset + 2] << 16) + ((int32_t)ltm_buffer[offset + 3] << 24);
}

int ltm_to_attitude(int v)
{
    if (v > 32767)
    {
        v = -1 * (65535 - v);
    }
    return v;
}

static void ltm_begin(uint16_t baud, uint16_t rx_pin, uint16_t tx_pin)
{
    ltm_serial.begin(baud, SWSERIAL_8N1, rx_pin, tx_pin, false);
}

static void ltm_end()
{
    ltm_serial.end();
}

static void ltm_loop()
{
    if (ltm_serial.available())
    {

        char data = ltm_serial.read();

        if (ltm_state == LTM_IDLE)
        {
            if (data == '$')
            {
                ltm_state = LTM_HEADER_START1;
            }
        }
        else if (ltm_state == LTM_HEADER_START1)
        {
            if (data == 'T')
            {
                ltm_state = LTM_HEADER_START2;
            }
            else
            {
                ltm_state = LTM_IDLE;
            }
        }
        else if (ltm_state == LTM_HEADER_START2)
        {
            ltm_frameType = data;
            ltm_state = LTM_HEADER_MSGTYPE;
            ltm_receiverIndex = 0;

            switch (data)
            {

            case 'G':
                ltm_frameLength = LTM_GFRAMELENGTH;
                break;
            case 'A':
                ltm_frameLength = LTM_AFRAMELENGTH;
                break;
            case 'S':
                ltm_frameLength = LTM_SFRAMELENGTH;
                break;
            case 'O':
                ltm_frameLength = LTM_OFRAMELENGTH;
                break;
            case 'N': // inav
                ltm_frameLength = LTM_NFRAMELENGTH;
                break;
            case 'X': // inav
                ltm_frameLength = LTM_XFRAMELENGTH;
                break;
            default:
                ltm_state = LTM_IDLE;
            }
        }
        else if (ltm_state == LTM_HEADER_MSGTYPE)
        {
            if (ltm_receiverIndex == ltm_frameLength - 4)
            {

                if (ltm_frameType == 'A')
                {
                    LTM_DATA.pitch = ltm_to_attitude(ltm_readInt_u16(0));
                    LTM_DATA.roll = ltm_to_attitude(ltm_readInt_u16(2));
                    LTM_DATA.heading = ltm_to_attitude(ltm_readInt_u16(4));
                }

                if (ltm_frameType == 'S')
                {
                    LTM_DATA.voltage = ltm_readInt(0);
                    LTM_DATA.rssi = ltm_readByte(4);

                    byte raw = ltm_readByte(6);
                    LTM_DATA.flightmode = raw >> 2;
                }

                if (ltm_frameType == 'G')
                {
                    LTM_DATA.latitude = ltm_readInt32(0);
                    LTM_DATA.longitude = ltm_readInt32(4);
                    LTM_DATA.groundSpeed = ltm_readByte(8);
                    LTM_DATA.altitude = ltm_readInt32(9);

                    uint8_t raw = ltm_readByte(13);
                    LTM_DATA.gpsSats = raw >> 2;
                    LTM_DATA.gpsFix = raw & 0x03;
                }

                if (ltm_frameType == 'X')
                {
                    LTM_DATA.hdop = ltm_readInt(0);
                    LTM_DATA.sensorStatus = ltm_readByte(2);
                }

                ltm_state = LTM_IDLE;
                memset(ltm_buffer, 0, LTM_LONGEST_FRAME_LENGTH);
            }
            else
            {
                /*
                 * If no, put data into buffer
                 */
                ltm_buffer[ltm_receiverIndex++] = data;
            }
        }
    }
}

#endif // eof