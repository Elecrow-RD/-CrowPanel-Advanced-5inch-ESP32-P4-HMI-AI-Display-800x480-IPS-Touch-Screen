/**
 * @file softap_sta.c
 * @brief Teaching source for 5inch_P4_IDF_17_WiFi_Functions.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/*  WiFi softAP & station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_ESP_WIFI_STA_SSID "mywifissid"
*/

/* STA Configuration */
#define EXAMPLE_ESP_WIFI_STA_SSID           "yanfa1"
#define EXAMPLE_ESP_WIFI_STA_PASSWD         "1223334444yanfa"
#define EXAMPLE_ESP_MAXIMUM_RETRY           CONFIG_ESP_MAXIMUM_STA_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WAPI_PSK
#endif

/* AP Configuration */
#define EXAMPLE_ESP_WIFI_AP_SSID            "ELECROW"
#define EXAMPLE_ESP_WIFI_AP_PASSWD          "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL            CONFIG_ESP_WIFI_AP_CHANNEL
#define EXAMPLE_MAX_STA_CONN                CONFIG_ESP_MAX_STA_CONN_AP


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/*DHCP server option*/
#define DHCPS_OFFER_DNS             0x02

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;

/**
 * @brief Translate a disconnect reason code into readable text.
 *
 * The reason code is the only clue the driver gives when the STA link cannot be
 * established, so it is logged in clear text (wrong SSID, wrong password, ...).
 * @param reason Reason code carried by WIFI_EVENT_STA_DISCONNECTED.
 * @return Constant string describing the reason.
 */
static const char *wifi_reason_to_str(uint8_t reason)
{
    if (reason == WIFI_REASON_NO_AP_FOUND) {
        return "no AP found: SSID typo, 5GHz-only router, or out of range";
    }
    if (reason == WIFI_REASON_AUTH_FAIL) {
        return "authentication failed: wrong password";
    }
    if (reason == WIFI_REASON_ASSOC_FAIL) {
        return "association refused: AP rejected the board (MAC filter / full)";
    }
    if (reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
        return "4-way handshake timeout: wrong password or PMF/WPA3 mismatch";
    }
    if (reason == WIFI_REASON_CONNECTION_FAIL) {
        return "connection failed";
    }
    if (reason == WIFI_REASON_BEACON_TIMEOUT) {
        return "beacon lost: signal too weak or AP went away";
    }
    if (reason == WIFI_REASON_AUTH_EXPIRE) {
        return "authentication expired";
    }
    if (reason == WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY) {
        return "AP found but its security mode is not supported";
    }
    if (reason == WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD) {
        return "AP found but weaker than the configured auth threshold";
    }
    if (reason == WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD) {
        return "AP found but RSSI below threshold";
    }
    return "unknown reason, see WIFI_REASON_xxx in esp_wifi_types.h";
}

/**
 * @brief Perform the wifi event handler operation.
 *
 * Called automatically when the associated driver or system event occurs.
 * @param arg Input or output value used by this operation.
 * @param event_base Input or output value used by this operation.
 * @param event_id Input or output value used by this operation.
 * @param event_data Input or output value used by this operation.
 * @return None.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" left, AID=%d, reason:%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG_STA, "Station started, connecting to SSID:%s ...", EXAMPLE_ESP_WIFI_STA_SSID);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *) event_data;
        ESP_LOGI(TAG_STA, "Associated with SSID:%.*s channel:%d bssid:"MACSTR" authmode:%d",
                 event->ssid_len, (char *)event->ssid, event->channel,
                 MAC2STR(event->bssid), event->authmode);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /* Without this branch the example blocks for ever in xEventGroupWaitBits()
         * whenever the uplink is unreachable: no "Got IP" line, no retry, no NAPT
         * and therefore no Internet for the phones on the softAP. */
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *) event_data;
        ESP_LOGW(TAG_STA, "Disconnected from SSID:%s, reason:%d (%s)",
                 EXAMPLE_ESP_WIFI_STA_SSID, event->reason, wifi_reason_to_str(event->reason));
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            s_retry_num++;
            ESP_LOGI(TAG_STA, "retry to connect to the AP (%d/%d)",
                     s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                     EXAMPLE_ESP_WIFI_STA_SSID, EXAMPLE_ESP_WIFI_STA_PASSWD);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG_STA, "Got IP:" IPSTR ", mask:" IPSTR ", gw:" IPSTR,
                 IP2STR(&event->ip_info.ip),
                 IP2STR(&event->ip_info.netmask),
                 IP2STR(&event->ip_info.gw));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGW(TAG_STA, "Lost IP address of the uplink");
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Log every AP that the Wi-Fi co-processor can currently see.
 *
 * The ESP32-P4 has no radio of its own: all Wi-Fi work is done by the ESP32-C6
 * over SDIO, so the scan result shows exactly what the uplink radio can reach.
 * It makes SSID typos, 5GHz-only routers and too-weak signals visible.
 * @return None.
 */
static void wifi_scan_dump(void)
{
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };

    if (esp_wifi_scan_start(&scan_config, true) != ESP_OK) {
        ESP_LOGE(TAG_STA, "Wi-Fi scan failed, cannot list the surrounding APs");
        return;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) {
        ESP_LOGE(TAG_STA, "Scan result: no 2.4GHz AP visible at all (antenna? co-processor?)");
        return;
    }
    if (ap_count > 20) {
        ap_count = 20;
    }

    wifi_ap_record_t *ap_list = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (ap_list == NULL) {
        ESP_LOGE(TAG_STA, "No memory for the scan result");
        return;
    }

    if (esp_wifi_scan_get_ap_records(&ap_count, ap_list) == ESP_OK) {
        bool ssid_found = false;
        ESP_LOGI(TAG_STA, "Scan result, %u AP(s) visible on 2.4GHz:", (unsigned)ap_count);
        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI(TAG_STA, "  %2d: SSID=\"%s\" ch=%d rssi=%d authmode=%d",
                     i + 1, (char *)ap_list[i].ssid, ap_list[i].primary,
                     ap_list[i].rssi, ap_list[i].authmode);
            if (strcmp((char *)ap_list[i].ssid, EXAMPLE_ESP_WIFI_STA_SSID) == 0) {
                ssid_found = true;
            }
        }
        if (ssid_found) {
            ESP_LOGW(TAG_STA, "SSID \"%s\" is visible: check the password "
                     "(and the router 5GHz/2.4GHz band steering settings)",
                     EXAMPLE_ESP_WIFI_STA_SSID);
        } else {
            ESP_LOGE(TAG_STA, "SSID \"%s\" is NOT in the list: check the spelling, or the "
                     "router really is 5GHz-only (ESP32-C6 is 2.4GHz only)",
                     EXAMPLE_ESP_WIFI_STA_SSID);
        }
    }
    free(ap_list);
}

/* Initialize soft AP */
/**
 * @brief Perform the wifi init softap operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_AP_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_AP_PASSWD,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_AP_PASSWD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD, EXAMPLE_ESP_WIFI_CHANNEL);

    return esp_netif_ap;
}

/* Initialize wifi station */
/**
 * @brief Perform the wifi init sta operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_netif_t *wifi_init_sta(void)
{
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_STA_SSID,
            .password = EXAMPLE_ESP_WIFI_STA_PASSWD,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
            * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

    return esp_netif_sta;
}

/**
 * @brief Perform the softap set dns addr operation.
 *
 * Called by the application when this module operation is required.
 * @param esp_netif_ap Input or output value used by this operation.
 * @param esp_netif_sta Input or output value used by this operation.
 * @return None.
 */
void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta)
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN,&dns);
    ESP_LOGI(TAG_AP, "DNS server learned from the uplink router: " IPSTR,
             IP2STR(&dns.ip.u_addr.ip4));
    if (dns.ip.u_addr.ip4.addr == 0) {
        /* Some routers answer the DHCP request without option 6 (DNS). The softAP would
         * then advertise 0.0.0.0 as DNS server: the phone gets an IP and can ping, but no
         * name can be resolved, which looks exactly like "connected but no Internet".
         * Fall back to a public DNS server so browsing keeps working. */
        esp_ip4_addr_t fallback;
        if (esp_netif_str_to_ip4("223.5.5.5", &fallback) == ESP_OK) {
            dns.ip.u_addr.ip4 = fallback;
            dns.ip.type = ESP_IPADDR_TYPE_V4;
            ESP_LOGW(TAG_AP, "Uplink router offered no DNS, using fallback " IPSTR,
                     IP2STR(&fallback));
        }
    }
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
    ESP_LOGI(TAG_AP, "softAP DHCP server now offers DNS " IPSTR
             " (reconnect the phone so that it takes a new lease)",
             IP2STR(&dns.ip.u_addr.ip4));
}

/**
 * @brief Start the lesson application.
 *
 * Called once by ESP-IDF after the system startup sequence completes.
 * @return None.
 */
void app_main(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    /* Initialize AP */
    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
    esp_netif_t *esp_netif_ap = wifi_init_softap();

    /* Initialize STA */
    ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
    esp_netif_t *esp_netif_sta = wifi_init_sta();

    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start() );

    /* Print the address of the softAP right away: the phone uses this subnet even
     * when the uplink is down, so the log is useful before any IP is obtained. */
    esp_netif_ip_info_t ap_info = {0};
    if (esp_netif_get_ip_info(esp_netif_ap, &ap_info) == ESP_OK) {
        ESP_LOGI(TAG_AP, "softAP \"%s\" gateway is " IPSTR "/" IPSTR,
                 EXAMPLE_ESP_WIFI_AP_SSID, IP2STR(&ap_info.ip), IP2STR(&ap_info.netmask));
    }

    /*
     * Wait until either the connection is established (WIFI_CONNECTED_BIT) or
     * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by event_handler() (see above).
     * The wait is bounded instead of portMAX_DELAY so that the "associated but the
     * router never answers DHCP" case is reported too instead of hanging silently.
     */
    EventBits_t bits = 0;
    bool stall_reported = false;
    while (bits == 0) {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(30000));
        if (bits == 0 && !stall_reported) {
            stall_reported = true;
            ESP_LOGW(TAG_STA, "No IP from SSID:%s after 30s: the board may be associated but "
                     "the router/DHCP server does not answer", EXAMPLE_ESP_WIFI_STA_SSID);
            wifi_scan_dump();
        }
    }

    /* xEventGroupWaitBits() returns the bits before the call returned,
     * hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_STA, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_STA_SSID, EXAMPLE_ESP_WIFI_STA_PASSWD);
        softap_set_dns_addr(esp_netif_ap,esp_netif_sta);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG_STA, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_STA_SSID, EXAMPLE_ESP_WIFI_STA_PASSWD);
        ESP_LOGW(TAG_STA, "The softAP stays up so the board is still reachable, but there is "
                 "no uplink and the phones on it have no Internet access.");
        /* Show what the radio can see: this immediately tells apart a typo in the SSID,
         * a 5GHz-only router and a signal that is simply too weak. */
        wifi_scan_dump();
    } else {
        ESP_LOGE(TAG_STA, "UNEXPECTED EVENT");
        return;
    }

    /* Set sta as the default interface */
    esp_netif_set_default_netif(esp_netif_sta);

    /* Enable napt on the AP netif */
    if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK) {
        ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", esp_netif_ap);
    } else {
        ESP_LOGI(TAG_AP, "NAPT enabled: traffic of the softAP clients is routed through the STA");
    }

    /* Final picture of the addresses, and a check for the classic "two interfaces in the
     * same subnet" mistake which silently breaks routing for the softAP clients. */
    esp_netif_ip_info_t sta_info = {0};
    esp_netif_get_ip_info(esp_netif_sta, &sta_info);
    ESP_LOGI(TAG_AP, "softAP " IPSTR "/" IPSTR "  <->  STA " IPSTR "/" IPSTR,
             IP2STR(&ap_info.ip), IP2STR(&ap_info.netmask),
             IP2STR(&sta_info.ip), IP2STR(&sta_info.netmask));
    if (sta_info.ip.addr != 0 && ap_info.ip.addr != 0 &&
        (sta_info.ip.addr & sta_info.netmask.addr) == (ap_info.ip.addr & ap_info.netmask.addr)) {
        ESP_LOGE(TAG_AP, "Subnet conflict: the router LAN and the softAP use the same subnet, "
                 "NAPT cannot work. Change the softAP address (e.g. to 192.168.5.1) or the "
                 "router LAN subnet.");
    }
}
