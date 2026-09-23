| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Wi-Fi SoftAP & Station Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This example demonstrates how to use the ESP Wi-Fi driver to act as both an Access Point and a Station simultaneously using the SoftAP and Station features.
With NAPT enabled on the softAP interface and the station interface set as the default interface this example can be used as Wifi nat router.

## How to use example
### Configure the project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Set the Wi-Fi SoftAP configuration.
    * Set `WiFi AP SSID`.
    * Set `WiFi AP Password`.

* Set the Wi-Fi STA configuration.
    * Set `WiFi Remote AP SSID`.
    * Set `WiFi Remote AP Password`.

Optional: If necessary, modify the other choices to suit your needs.

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the serial output:

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

## Example Output

There is the console output for this example:

```
I (680) WiFi SoftAP: ESP_WIFI_MODE_AP
I (690) WiFi SoftAP: wifi_init_softap finished. SSID:myssid password:mypassword channel:1
I (690) WiFi Sta: ESP_WIFI_MODE_STA
I (690) WiFi Sta: wifi_init_sta finished.
I (700) phy_init: phy_version 4670,719f9f6,Feb 18 2021,17:07:07
I (800) wifi:mode : sta (58:bf:25:e0:41:00) + softAP (58:bf:25:e0:41:01)
I (800) wifi:enable tsf
I (810) wifi:Total power save buffer number: 16
I (810) wifi:Init max length of beacon: 752/752
I (810) wifi:Init max length of beacon: 752/752
I (820) WiFi Sta: Station started
I (820) wifi:new:<1,1>, old:<1,1>, ap:<1,1>, sta:<1,1>, prof:1
I (820) wifi:state: init -> auth (b0)
I (830) wifi:state: auth -> assoc (0)
E (840) wifi:Association refused temporarily, comeback time 1536 mSec
I (2380) wifi:state: assoc -> assoc (0)
I (2390) wifi:state: assoc -> run (10)
I (2400) wifi:connected with myssid_c3, aid = 1, channel 1, 40U, bssid = 84:f7:03:60:86:1d
I (2400) wifi:security: WPA2-PSK, phy: bgn, rssi: -14
I (2410) wifi:pm start, type: 1

I (2410) wifi:AP's beacon interval = 102400 us, DTIM period = 2
I (3920) WiFi Sta: Got IP:192.168.5.2
I (3920) esp_netif_handlers: sta ip: 192.168.5.2, mask: 255.255.255.0, gw: 192.168.5.1
I (3920) WiFi Sta: connected to ap SSID:myssid_c3 password:mypassword_c3
```

## Troubleshooting

### No "Got IP" line and the phones on the softAP have no Internet

On the ESP32-P4 the Wi-Fi driver does not run on the P4 itself: it is executed by an
ESP32-C6 (ESP-Hosted co-processor) that talks to the P4 over SDIO. The P4 only runs the
TCP/IP stack, the softAP DHCP server and NAPT.

Consequences worth remembering while reading the log:

1. `Got IP:` is printed by the STA netif, so it only appears when the uplink router really
   answered DHCP. `NAPT` and the DNS option for the softAP DHCP server are configured
   *after* that event; as long as the uplink is down, the softAP is up but its clients
   cannot reach anything.
2. The STA part can only use **2.4 GHz**. A 5 GHz-only SSID looks exactly like a wrong SSID
   (`reason:201 no AP found`).
3. `WIFI_EVENT_STA_DISCONNECTED` must be handled, otherwise the example silently blocks in
   `xEventGroupWaitBits()` and nothing is printed at all. The handler in `softap_sta.c`
   now logs the reason code and performs the retries.

Log lines to look for:

```
I (xxx) WiFi Sta: Station started, connecting to SSID:yanfa1 ...
W (xxx) WiFi Sta: Disconnected from SSID:yanfa1, reason:201 (no AP found: SSID typo, 5GHz-only router, or out of range)
I (xxx) WiFi Sta: retry to connect to the AP (1/5)
...
E (xxx) WiFi Sta: Failed to connect to SSID:yanfa1, password:...
I (xxx) WiFi Sta: Scan result, 12 AP(s) visible on 2.4GHz:
I (xxx) WiFi Sta:    1: SSID="yanfa1" ch=6 rssi=-45 authmode=3
```

Typical causes and checks:

| Symptom | Cause | Fix |
| --- | --- | --- |
| `reason:201`, SSID missing in the scan dump | SSID typo, or the router only broadcasts on 5 GHz | Use the exact 2.4 GHz SSID (`EXAMPLE_ESP_WIFI_STA_SSID` / `EXAMPLE_ESP_WIFI_STA_PASSWD`) |
| `reason:202` / `reason:204` | Wrong password, or WPA3/PMF-only AP | Correct the password; try `WPA2/WPA3 PSK` auth threshold in `menuconfig` |
| SSID visible, RSSI < -80 | Signal too weak | Move the board closer to the router |
| `Got IP` printed, phone still has no Internet | The softAP subnet equals the router LAN subnet | The log prints a `Subnet conflict` error; change the softAP address or the router LAN |
| `Got IP` printed, phone resolves nothing | The phone still holds an old DHCP lease without DNS | Forget/reconnect the "ELECROW" network after the board printed `Got IP` |
| No `WiFi Sta`/`WiFi SoftAP` lines at all | ESP-Hosted link not up (co-processor firmware missing or version mismatch) | Check the boot log for `hosted`/`transport` errors and re-flash the ESP32-C6 slave firmware of the matching `esp_hosted` version |

### Changing the SSID / password without editing the code

The credentials can be edited directly at the top of `main/softap_sta.c`
(`EXAMPLE_ESP_WIFI_STA_SSID`, `EXAMPLE_ESP_WIFI_STA_PASSWD`, `EXAMPLE_ESP_WIFI_AP_SSID`),
or through `idf.py menuconfig` -> `Example Configuration`.

## Reporting issues

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
