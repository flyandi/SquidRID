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
#ifndef SQUID_TOOLS_H
#define SQUID_TOOLS_H

typedef struct {
  double lat = 0.0;
  double lon = 0.0;
} LatLon_t;

class Squid_Tools {

public:
  Squid_Tools(void);

  void setupTime();
  void setupTime(int hour, int day, int month, int year);

  void calc_m_per_deg(double, double, double *, double *);
  void calc_m_per_deg(double, double *, double *);

  int check_EU_op_id(const char *, const char *);
  char luhn36_check(const char *);
  int luhn36_c2i(char);
  char luhn36_i2c(int);

  void haversineDistance(LatLon_t, double, int, LatLon_t *);
  bool haversineAt(LatLon_t, double, double, int, unsigned long, LatLon_t *);

  void generateRandomPointInCircle(double, double, double, LatLon_t *);
  void generateMAC(uint8_t mac[6]);

private:
  char s[20];
};

#endif
