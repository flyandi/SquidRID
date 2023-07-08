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

// Original by Steve Jack / Rewritten for multi RID simulation

#pragma GCC diagnostic warning "-Wunused-variable"

#define DIAGNOSTICS 0

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include "squid_tools.h"
#include <math.h>

const double R = 6371e3;  // Earth's radius in kilometers
const double RM = 6371000.0;

Squid_Tools::Squid_Tools() {
  memset(s, 0, sizeof(s));
  return;
}

bool Squid_Tools::haversineAt(LatLon_t origin, double heading, double speed, int distance, unsigned long ms, LatLon_t *out) {
  bool b = false;
  int distance_traveled = speed * (ms / 1000);

  if (distance_traveled >= distance) {
    distance_traveled = distance;
    b = true;  // reached
  }

  haversineDistance(origin, heading, distance_traveled, out);
  return b;
}

void Squid_Tools::haversineDistance(LatLon_t origin, double heading, int distance, LatLon_t *out) {
  origin.lat = origin.lat * M_PI / 180;
  origin.lon = origin.lon * M_PI / 180;
  heading = heading * M_PI / 180;
  double D = (double)distance;
  out->lat = asin(sin(origin.lat) * cos(D / R) + cos(origin.lat) * sin(D / R) * cos(heading));
  out->lon = origin.lon + atan2(sin(heading) * sin(D / R) * cos(origin.lat), cos(D / R) - sin(origin.lat) * sin(out->lat));
  out->lat = out->lat * 180 / M_PI;
  out->lon = out->lon * 180 / M_PI;
}

void Squid_Tools::setupTime(int hour, int day, int month, int year) {

  time_t time_2;
  struct tm clock_tm;
  struct timeval tv = { 0, 0 };
  struct timezone utc = { 0, 0 };
  memset(&clock_tm, 0, sizeof(struct tm));
  clock_tm.tm_hour = hour;
  clock_tm.tm_mday = day;
  clock_tm.tm_mon = month;
  clock_tm.tm_year = year - 1900;
  tv.tv_sec = time_2 = mktime(&clock_tm);
  settimeofday(&tv, &utc);
  delay(500);
  srand(micros());
}

void Squid_Tools::setupTime() {
  setupTime(10, 17, 1, 2023);
}

/*
 *
 */

void Squid_Tools::calc_m_per_deg(double lat_d, double long_d, double *m_deg_lat, double *m_deg_long) {

  calc_m_per_deg(lat_d, m_deg_lat, m_deg_long);

  return;
}

//

void Squid_Tools::calc_m_per_deg(double lat_d, double *m_deg_lat, double *m_deg_long) {

  double pi, deg2rad, sin_lat, cos_lat;

  pi = 4.0 * atan(1.0);
  deg2rad = pi / 180.0;

  lat_d *= deg2rad;

  sin_lat = sin(lat_d);
  cos_lat = cos(lat_d);

#if 1  // Wikipedia

  double a = 0.08181922, b, radius;

  b = a * sin_lat;
  radius = 6378137.0 * cos_lat / sqrt(1.0 - (b * b));
  *m_deg_long = deg2rad * radius;
  *m_deg_lat = 111132.954 - (559.822 * cos(2.0 * lat_d)) - (1.175 * cos(4.0 * lat_d));

#else  // Astronomical Algorithms

  double a = 6378140.0, c, d, e = 0.08181922, rho, Rp = 0.0, Rm = 0.0;

  rho = 0.9983271 + (0.0016764 * cos(2.0 * lat_d)) - (0.0000035 * cos(4.0 * lat_d));
  c = e * sin_lat;
  d = sqrt(1.0 - (c * c));
  Rp = a * cos_lat / d;
  *m_deg_long = deg2rad * Rp;
  Rm = a * (1.0 - (e * e)) / pow(d, 3);
  *m_deg_lat = deg2rad * Rm;

#endif

  return;
}

/*
 *
 */

int Squid_Tools::check_EU_op_id(const char *id, const char *secret) {

  int i, j;
  char check;

  if ((strlen(id) != 16) && (strlen(secret) != 3)) {

    return 0;
  }

  for (i = 0, j = 0; i < 12; ++i) {

    s[j++] = id[i + 3];
  }

  for (i = 0; i < 3; ++i) {

    s[j++] = secret[i];
  }

  s[j] = 0;

  check = luhn36_check(s);

  return ((id[15] == check) ? 1 : 0);
}

/*
 *
 */

char Squid_Tools::luhn36_check(const char *s) {

  int sum = 0, factor = 2, l, i, add, rem;
  const int base = 36;

  l = strlen(s);

  for (i = l - 1; i >= 0; --i) {

    add = luhn36_c2i(s[i]) * factor;
    sum += (add / base) + (add % base);

    factor = (factor == 2) ? 1 : 2;
  }

  rem = sum % base;

  return luhn36_i2c(base - rem);
}

/*
 *
 */

int Squid_Tools::luhn36_c2i(char c) {

  if ((c >= '0') && (c <= '9')) {

    return (c - '0');
  } else if ((c >= 'a') && (c <= 'z')) {

    return (10 + (c - 'a'));
  } else if ((c >= 'A') && (c <= 'Z')) {

    return (10 + (c - 'A'));
  }

  return 0;
}

/*
 *
 */

char Squid_Tools::luhn36_i2c(int i) {

  if ((i >= 0) && (i <= 9)) {

    return ('0' + i);
  } else if ((i >= 10) && (i < 36)) {

    return ('a' + i - 10);
  }

  return '0';
}


void Squid_Tools::generateRandomPointInCircle(double lat, double lng, double radius, LatLon_t *out) {
  double u = random(0, RAND_MAX) / double(RAND_MAX);  // Random value between 0 and 1
  double v = random(0, RAND_MAX) / double(RAND_MAX);  // Random value between 0 and 1
  double w = radius * sqrt(u);
  double t = 2.0 * M_PI * v;
  double x = w * cos(t);
  double y = w * sin(t);
  out->lat = lat + (y / RM) * (180.0 / M_PI);
  out->lon = lng + (x / (RM * cos(lat * M_PI / 180.0))) * (180.0 / M_PI);
}

void Squid_Tools::generateMAC(uint8_t mac[6]) {
  for (int i = 0; i < 6; i++) {
    mac[i] = (uint8_t)random(0, 256);
  }
  mac[0] = mac[0] & 0xfe;  // Ensure first digit is not multicast
}
