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
#define DIAGNOSTICS 1

//
#pragma GCC diagnostic warning "-Wunused-variable"

#include <Arduino.h>
#include "squid_instance.h"

#if USE_WIFI

#include <WiFi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs_flash.h>

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

esp_err_t event_handler(void *, system_event_t *);

#if USE_NATIVE_WIFI == 0
static const char *password = "password";
#endif

#endif // WIFI

#if USE_BT

#include "BLEDevice.h"
#include "BLEUtils.h"

static esp_ble_adv_data_t advData;
static esp_ble_adv_params_t advParams;
static BLEUUID service_uuid;

#endif // BT

static Stream *Debug_Serial = NULL;
static bool construct2Init = false;

/*
 *
 */

void construct2()
{

  if (construct2Init)
  {
    return;
  }

  construct2Init = true;

#if USE_BT

  memset(&advData, 0, sizeof(advData));

  advData.set_scan_rsp = false;
  advData.include_name = false;
  advData.include_txpower = false;
  advData.min_interval = 0x0006;
  advData.max_interval = 0x0050;
  advData.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

  memset(&advParams, 0, sizeof(advParams));

  advParams.adv_int_min = 0x0020;
  advParams.adv_int_max = 0x0040;
  advParams.adv_type = ADV_TYPE_IND;
  advParams.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  advParams.channel_map = ADV_CHNL_ALL;
  advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
  advParams.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;

  service_uuid = BLEUUID("0000fffa-0000-1000-8000-00805f9b34fb");

#endif // USE_BT

  return;
}

/*
 *
 */

void init2(char *ssid, int ssid_length, uint8_t *WiFi_mac_addr, uint8_t wifi_channel)
{

  int status;
  char text[128];

  status = 0;
  text[0] = text[63] = 0;

#if DIAGNOSTICS
  Debug_Serial = &Serial;
#endif

#if USE_WIFI

  int8_t wifi_power;
  wifi_config_t ap_config;
  // static wifi_country_t country = {"GB", 1, 13, 20, WIFI_COUNTRY_POLICY_AUTO};
  static wifi_country_t country{"US", 1, 11, WIFI_COUNTRY_POLICY_AUTO};

  memset(&ap_config, 0, sizeof(ap_config));

#if USE_NATIVE_WIFI

  WiFi.softAP(ssid, "password", wifi_channel);

  esp_wifi_get_config(WIFI_IF_AP, &ap_config);

  // ap_config.ap.ssid_hidden = 1;
  status = esp_wifi_set_config(WIFI_IF_AP, &ap_config);

#else

  wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();

  nvs_flash_init();
  tcpip_adapter_init();

  esp_event_loop_init(event_handler, NULL);
  esp_wifi_init(&init_cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);

  strcpy((char *)ap_config.ap.ssid, ssid);
  strcpy((char *)ap_config.ap.password, password);
  ap_config.ap.ssid_len = strlen(ssid);
  ap_config.ap.channel = (uint8_t)wifi_channel;
  ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  ap_config.ap.ssid_hidden = 0;
  ap_config.ap.max_connection = 4;
  ap_config.ap.beacon_interval = 1000; // Pass beacon_interval from id_open.cpp?

  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  esp_wifi_set_ps(WIFI_PS_NONE);

#endif

  esp_wifi_set_country(&country);
  status = esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

  // esp_wifi_set_max_tx_power(78);
  esp_wifi_get_max_tx_power(&wifi_power);

  status = esp_read_mac(WiFi_mac_addr, ESP_MAC_WIFI_STA);

  if (Debug_Serial)
  {

    sprintf(text, "esp_read_mac():  %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            WiFi_mac_addr[0], WiFi_mac_addr[1], WiFi_mac_addr[2],
            WiFi_mac_addr[3], WiFi_mac_addr[4], WiFi_mac_addr[5]);
    Debug_Serial->print(text);
    // power <= 72, dbm = power/4, but 78 = 20dbm.
    sprintf(text, "max_tx_power():  %d dBm\r\n", (int)((wifi_power + 2) / 4));
    Debug_Serial->print(text);
  }

#endif // WIFI

#if USE_BT

  int power_db;
  esp_power_level_t power;

  BLEDevice::init(ssid);

  // Using BLEDevice::setPower() seems to have no effect.
  // ESP_PWR_LVL_N12 ...  ESP_PWR_LVL_N0 ... ESP_PWR_LVL_P9

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

  power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
  power_db = 3 * ((int)power - 4);

#endif

  return;
}

/*
 *
 */

int transmit_wifi2(uint8_t *buffer, int length)
{

  esp_err_t wifi_status = 0;

#if USE_WIFI

  if (length)
  {

    wifi_status = esp_wifi_80211_tx(WIFI_IF_AP, buffer, length, true);
  }

#endif

  return (int)wifi_status;
}

/*
 *
 */

int transmit_ble2(uint8_t *ble_message, int length)
{

  esp_err_t ble_status;
  static int advertising = 0;

#if USE_BT

  if (advertising)
  {

    ble_status = esp_ble_gap_stop_advertising();
  }

  ble_status = esp_ble_gap_config_adv_data_raw(ble_message, length);
  ble_status = esp_ble_gap_start_advertising(&advParams);

  advertising = 1;

#endif // BT

  return (int)ble_status;
}

/*
 *
 */

#if USE_WIFI

esp_err_t event_handler(void *ctx, system_event_t *event)
{

  return ESP_OK;
}

#endif
