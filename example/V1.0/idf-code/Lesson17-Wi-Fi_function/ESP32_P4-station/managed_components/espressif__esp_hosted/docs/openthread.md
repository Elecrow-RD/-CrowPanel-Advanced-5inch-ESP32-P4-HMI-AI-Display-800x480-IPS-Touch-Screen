# OpenThread Support

**Table of Contents**

<details>

- [1. Introduction](#1-introduction)
  - [1.1 Prerequisites](#11-prerequisites)
	- [1.1.1 Basic OpenThread](#111-basic-openthread)
	- [1.1.2 Operating as an OpenThread Border Router](#112-operating-as-an-openthread-border-router)
- [2. Configuration](#2-configuration)
  - [2.1 OpenThread RCP Configuration on the Co-processor](#21-openthread-rcp-configuration-on-the-co-processor)
  - [2.2 OpenThread Configuration on the Host](#22-openthread-configuration-on-the-host)
	- [2.2.1 Initialising the OpenThread Connection from the Host Application](#221-initialising-the-openthread-connection-from-the-host-application)
- [3. More Information](#3-more-information)

</details>

## 1. Introduction

OpenThread is an IP stack running on the 802.15.4 MAC layer which features mesh network and low power consumption.

ESP-Hosted supports OpenThread by running the OpenThread Host on the Host Processor and the OpenThread RCP (Radio Co-Processor) on the co-processor. The diagram below shows how the OpenThread Host and RCP communicate.

```
    +------------+                                      +------------+
    |            |         ESP-Hosted Transport         |            |
    | OpenThread |<------------------------------------>| OpenThread | (Example: ESP32-C6)
    | Host       |                                      | RCP        |
    |            |                                      |            |
    |            |                 UART                 |            |
    |            |<------------------------------------>|            |
    |            |                                      |            |
    +------------+                                      +------------+
```

> [!NOTE]
> Currently a dedicated UART channel is required to pass OpenThread data between the Host and RCP. OpenThread over the current ESP-Hosted Transport to be supported in the future.

### 1.1 Prerequisites

#### 1.1.1 Basic OpenThread

The co-processor must support OpenThread RCP operation.

Example co-processors that support OpenThread RCP include the ESP32-H2, ESP32-C5 and ESP32-C6.

#### 1.1.2 Operating as an OpenThread Border Router

The co-processor must support OpenThread RCP operation and Wi-Fi to operate as part of an OpenThread Border Router, which passes OpenThread packets between the OpenThread and Wi-Fi networks.

Example co-processors as RCPs that support OpenThread Border Router operation include the ESP32-C5 and ESP32-C6.

**Coexistence between OpenThread and Wi-Fi**

The co-processor has only one hardware radio, so Wi-Fi and OpenThread have to share the radio (coexistence). This may lead to performance issues or dropped packets if there is a lot of traffic on Wi-Fi and/or OpenThread.

An alternative option is to use two co-processors: one for Wi-Fi and one for OpenThread:

```
    +------------+
    |            |         ESP-Hosted Transport         +------------+
    | OpenThread |<------------------------------------>| Wi-Fi      | (Example: ESP32-C6)
    | Border     |                                      +------------+
    | Router     |
    | Host       |                 UART                 +------------+
    |            |<------------------------------------>| OpenThread | (Example: ESP32-H2)
    |            |                                      | RCP        |
    +------------+                                      +------------+
```

Here, OpenThread is operating independent of ESP-Hosted. Since there are now two hardware radios, coexistence is not required.

More information on this mode of operation for the ESP32-P4 can be found in the [ESP-IDF OpenThread Border Router Example for ESP32-P4](https://github.com/espressif/esp-thread-br/blob/main/examples/basic_thread_border_router/README_esp32p4.md).

## 2. Configuration

### 2.1 OpenThread RCP Configuration on the Co-processor

> [!NOTE]
> This section targets the ESP32-C6 as the RCP. The same configuration is required when using other supported co-processors as the OpenThread RCP.

In the ESP-Hosted project `slave` directory:

Edit `sdkconfig.defaults.esp32c6` to add or enable the OpenThread section. These settings are required to enable OpenThread, configure it to be an RCP, and to disable OpenThread features not required for a RCP.

```text
CONFIG_OPENTHREAD_ENABLED=y
CONFIG_OPENTHREAD_RADIO=y
CONFIG_OPENTHREAD_DIAG=n
CONFIG_OPENTHREAD_COMMISSIONER=n
CONFIG_OPENTHREAD_JOINER=n
CONFIG_OPENTHREAD_BORDER_ROUTER=n
CONFIG_OPENTHREAD_CLI=n
CONFIG_OPENTHREAD_SRP_CLIENT=n
CONFIG_OPENTHREAD_DNS_CLIENT=n
CONFIG_OPENTHREAD_TASK_SIZE=3072
CONFIG_OPENTHREAD_CONSOLE_ENABLE=n
CONFIG_OPENTHREAD_LOG_LEVEL_DYNAMIC=n

CONFIG_ESP_COEX_SW_COEXIST_ENABLE=y
```

On the command-line:

```bash
idf.py set-target esp32c6
idf.py menuconfig
```

Enter the OpenThread section to configure the dedicated OpenThread UART:


```
Example Configuration
└── [*] Enable OpenThread RCP (Radio Co-Processor)
    └── OpenThread RCP Configuration
        ├── OpenThread Transport ──> UART
        └── (configure UART parameters)
```

If you did not edit `sdkconfig.defaults.esp32c6` to enable OpenThread (above), modify these settings first:

```
Component config
├── Wireless Coexistence
│   └── [*] Software controls WiFi/Bluetooth coexistence
└── OpenThread
    ├── [*] OpenThread
    ├── Thread Task Parameters
    │   └── (3072) Size of OpenThread task
    ├── Thread Console
    │   ├── [ ] Enable OpenThread console
    │   └── [ ] Enable Openthread Command-Line Interface
    ├── Thread Core Features
    │   ├── Thread device type ──> Radio Only Device
    │   ├── [ ] Enable Commissioner
    │   ├── [ ] Enable Joiner
    │   ├── [ ] Enable SRP Client
    │   ├── [ ] Enable DNS Client
    │   ├── [ ] Enable diag Client
    └── Thread Log
        └── [ ] Enable dynamic log level control
```

You can now build the co-processor as an OpenThread RCP.

### 2.2 OpenThread Configuration on the Host

> [!NOTE]
> This section targets the ESP32-P4 as the OpenThread Host.

OpenThread configuration on the host depends on the OpenThread features to be enabled. ESP-Hosted includes two examples:

- [OpenThread as a Border Router](https://github.com/espressif/esp-hosted-mcu/tree/main/examples/host_openthread_border_router)
- [OpenThread with CLI interface](https://github.com/espressif/esp-hosted-mcu/tree/main/examples/host_openthread_cli)

Further OpenThread Host examples can be found in the [ESP-IDF Examples directory](https://github.com/espressif/esp-idf/tree/master/examples/openthread).

**Further configuration**

PSRAM should be enabled to run OpenThread with ESP-Hosted due to memory requirements.

The OpenThread host must also be configured to recognise that the OpenThread RCP has coexistence enabled: otherwise OpenThread initialisation will fail due to a mismatch of OpenThread capabilities between the Host and RCP.

```text
### enable SPIRAM, else application will not start (out of memory)
CONFIG_SPIRAM=y
CONFIG_ESP_HOSTED_DFLT_TASK_FROM_SPIRAM=y

### SW coexistence on the co-processor is on, so this option must be
### disabled on host to keep OpenThread capabilities consistent
CONFIG_OPENTHREAD_RX_ON_WHEN_IDLE=n
```

For the ESP32-P4, these have been set-up in the `sdkconfig.default.esp32p4` configuration files in the ESP-Hosted OpenThread examples.

### 2.2.1 Initialising the OpenThread Connection from the Host Application

Initialise the ESP-Hosted interface first:

```c
esp_hosted_init();
esp_hosted_connect_to_slave();
```

Then initialise the OpenThread RCP using ESP-Hosted:

```c
esp_hosted_openthread_rcp_init();
esp_hosted_openthread_rcp_start();
```

In ESP-IDF, the OpenThread Host interface needs to know the OpenThread radio configuration. This is provided via ESP-Hosted:

```c
esp_hosted_openthread_radio_config_t hosted_radio_config = { 0 };
esp_hosted_openthread_get_radio_config(&hosted_radio_config);
```

`hosted_radio_config` is then used to configure the OpenThread's radio configuration.

See the ESP-Hosted OpenThread examples for more information.

## 3. More Information

- [ESP-IDF OpenThread documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/openthread.html)
- [ESP-IDF Coexistence Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-guides/coexist.html)
- [ESP-IDF OpenThread Border Router Example for ESP32-P4 with two co-processors](https://github.com/espressif/esp-thread-br/blob/main/examples/basic_thread_border_router/README_esp32p4.md)
