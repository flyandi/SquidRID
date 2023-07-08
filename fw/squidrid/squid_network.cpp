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
#include "squid_network.h"

Squid_Network::Squid_Network()
{
    // nothing
}

void Squid_Network::begin()
{
    begin(SD_NETWORK_MODE_HYBRID);
}

void Squid_Network::begin(Squid_Network_Mode_t m)
{
    mode = m;
    if (mode == SD_NETWORK_MODE_HYBRID || mode == SD_NETWORK_MODE_BT)
    {
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
    }

    return;
}

void Squid_Network::setWifiDriver(int driver_id)
{
    wifi_driver = driver_id;
}

bool Squid_Network::enqueue(Squid_Network_Message message)
{
    if (queue_index >= SD_NETWORK_QUEUE_SIZE)
    {
        queue_index = 0; // circular
    }
    message.placed = 1;
    queue[queue_index] = message;
    queue_index++;
    return true;
}

bool Squid_Network::dequeue(Squid_Network_Message *message)
{
    int index = queue_index - 1 < 0 ? SD_NETWORK_QUEUE_SIZE - 1 : queue_index - 1;
    *message = queue[index];
    queue[index].placed = 0;
    return message->placed == 1;
}

bool Squid_Network::addMessage(uint8_t mac[6], char ssid[32], int ssid_length, uint8_t *buffer, int length)
{
    Squid_Network_Message message;
    memcpy(message.mac, mac, sizeof(mac));
    strncpy(message.ssid, ssid, sizeof(ssid));
    message.ssid_length = ssid_length;
    message.buffer = buffer;
    message.length = length;
    message.placed = 1;
    return enqueue(message);
}

void Squid_Network::loop()
{
    if(millis() - msg_last > SD_NETWORK_PULSE) {
        
        Squid_Network_Message message;
        if (dequeue(&message))
        {
            transmit_bt(&message);
        }
        msg_last = millis();
    }
}

void Squid_Network::transmit_bt(Squid_Network_Message *message)
{
    int power_db;
    esp_power_level_t power;
    esp_err_t ble_status;

    if (bt_ok == 1)
    {

        BLEDevice::deinit(false);
        bt_ok = 0;
    }

    if (bt_ok == 0)
    {
        esp_base_mac_addr_set(message->mac);
        BLEDevice::init(message->ssid);
        bt_ok = 1;
    }

    // Using BLEDevice::setPower() seems to have no effect.
    // ESP_PWR_LVL_N12 ...  ESP_PWR_LVL_N0 ... ESP_PWR_LVL_P9
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    power = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
    power_db = 3 * ((int)power - 4);

    if (bt_running == 1)
    {
        ble_status = esp_ble_gap_stop_advertising();
        bt_running = 0;
    }

    ble_status = esp_ble_gap_config_adv_data_raw(message->buffer, message->length);
    ble_status = esp_ble_gap_start_advertising(&advParams);
    bt_running = 1;

    return;
}

void Squid_Network::transmit_wifi(Squid_Network_Message *message)
{
    /*
    int8_t wifi_power;
    wifi_config_t ap_config;
    wifi_country_t country{"US", 1, 11, WIFI_COUNTRY_POLICY_AUTO};

    memset(&ap_config, 0, sizeof(ap_config));

    if (wifi_driver == 1)
    {
        WiFi.softAP(ssid, "password", wifi_channel);
        esp_wifi_get_config(WIFI_IF_AP, &ap_config);
        // ap_config.ap.ssid_hidden = 1;
        status = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    }
    else
    {
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
    }

    esp_wifi_set_country(&country);
    status = esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

    // esp_wifi_set_max_tx_power(78);
    esp_wifi_get_max_tx_power(&wifi_power);

    status = esp_read_mac(WiFi_mac_addr, ESP_MAC_WIFI_STA);
    // esp_base_mac_addr_set(new_mac);

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
*/
    return;
}
