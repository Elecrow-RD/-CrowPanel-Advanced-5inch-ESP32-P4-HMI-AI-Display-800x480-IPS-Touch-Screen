/**
 * @file bsp_wireless.cpp
 * @brief Provides the board-support interface used to control a lesson peripheral.
 *
 * This file belongs to the Lesson14_RX_SX1262_Wireless_Module course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_wireless.h"
#include <RadioLib.h>
#include "EspHal.h"
#include <stdio.h>
#include <string.h>

#if (RADIOLIB_VERSION_MAJOR > 7) || ((RADIOLIB_VERSION_MAJOR == 7) && (RADIOLIB_VERSION_MINOR >= 7))
#define BSP_RADIOLIB_HAS_CONFIG_API 1
#else
#define BSP_RADIOLIB_HAS_CONFIG_API 0
#endif

#if CONFIG_BSP_SX1262_ENABLED
static int16_t begin_sx1262_radio(SX1262 *radio, float tcxo_voltage)
{
#if BSP_RADIOLIB_HAS_CONFIG_API
  ConfigLoRa_t config;
  config.frequency = 915.0;
  config.bandwidth = 125.0;
  config.spreadingFactor = 7;
  config.codingRate = 7;
  config.syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
  config.power = 22;
  config.preambleLength = 8;
  radio->tcxoVoltage = tcxo_voltage;
  return radio->begin(config);
#else
  return radio->begin(915.0, 125.0, 7, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                      22, 8, tcxo_voltage, false);
#endif
}
#endif

#if CONFIG_BSP_NRF2401_ENABLED
static int16_t begin_nrf24_radio(nRF24 *radio)
{
#if BSP_RADIOLIB_HAS_CONFIG_API
  ConfigFSK_t config;
  config.frequency = 2400;
  config.bitRate = 250;
  config.power = 0;
  radio->addressWidth = 5;
  return radio->begin(config);
#else
  return radio->begin(2400, 250, 0, 5);
#endif
}
#endif
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#if CONFIG_BSP_SX1262_ENABLED
class BSP_SX1262
{
public:
  BSP_SX1262() {};

  ~BSP_SX1262() {};

  esp_err_t Sx1262_tx_init();

  void Sx1262_tx_deinit();

  bool Send_pack_radio();

  esp_err_t Sx1262_rx_init();

  void Sx1262_rx_deinit();

  void Received_pack_radio(size_t len);

protected:
private:
  static Module *bsp_sx_mod;
  static SX1262 *bsp_sx_radio;
};

EspHal lora_hal;
Module *BSP_SX1262::bsp_sx_mod = nullptr;
SX1262 *BSP_SX1262::bsp_sx_radio = nullptr;

static int lora_transmissionState = RADIOLIB_ERR_NONE;
volatile bool lora_transmittedFlag = true;
volatile bool lora_receivedFlag = false;
static size_t lora_received_len = 0;

// Pointer to the data reception callback function
static void (*rx_data_callback)(const char* data, size_t len, float rssi, float snr) = NULL;
#endif

#if CONFIG_BSP_NRF2401_ENABLED
class BSP_NRF2401
{
public:
  BSP_NRF2401() {};

  ~BSP_NRF2401() {};

  esp_err_t NRF24_tx_init();

  void NRF24_tx_deinit();

  bool Send_pack_radio();

  esp_err_t NRF24_rx_init();

  void NRF24_rx_deinit();

  void Received_pack_radio(size_t len);

protected:
private:
  static Module *bsp_nrf_mod;
  static nRF24 *bsp_nrf_radio;
};

EspHal nrf_hal;
Module *BSP_NRF2401::bsp_nrf_mod = nullptr;
nRF24 *BSP_NRF2401::bsp_nrf_radio = nullptr;

static int radio24_transmissionState = RADIOLIB_ERR_NONE;
volatile bool radio24_transmittedFlag = true;
volatile bool radio24_receivedFlag = false;
#endif
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
// --------------------------------------------------------------------------------
#if CONFIG_BSP_SX1262_ENABLED

static uint32_t sx1262_tx_counter = 0;

/**
 * @brief Apply the requested set SX1262 transmit flag operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Nothing.
 */
static void set_sx1262_tx_flag(void)
{
  lora_transmittedFlag = true;
}

/**
 * @brief Initialize SX1262 transmit init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
esp_err_t BSP_SX1262::Sx1262_tx_init()
{
  lora_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
  lora_hal.setSpiFrequency(8000000);
  lora_hal.spiBegin();

  bsp_sx_mod = new Module(&lora_hal, SX1262_GPIO_NSS, SX1262_GPIO_IRQ, SX1262_GPIO_NRST, SX1262_GPIO_BUSY);
  bsp_sx_radio = new SX1262(bsp_sx_mod);
  int state = begin_sx1262_radio(bsp_sx_radio, 3.3);
  if (state != RADIOLIB_ERR_NONE)
  {
    SX1262_ERROR("radio tx init failed, code :%d", state);
    lora_hal.spiEnd();
    return ESP_FAIL;
  }
  // bsp_sx_radio->setCurrentLimit(60);
  bsp_sx_radio->setPacketSentAction(set_sx1262_tx_flag);
  return ESP_OK;
}

/**
 * @brief Initialize SX1262 transmit deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
void BSP_SX1262::Sx1262_tx_deinit()
{
  lora_transmittedFlag = true;
  bsp_sx_radio->finishTransmit();
  bsp_sx_radio->clearPacketSentAction();
  bsp_sx_radio->standby();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  lora_hal.spiEnd();
}

/**
 * @brief Apply the requested send pack radio operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
bool BSP_SX1262::Send_pack_radio()   // Function to handle LoRa packet transmission, returns true if transmission occurred successfully
{
  static char text[32];   // Static buffer to store the text message for transmission

  if (lora_transmittedFlag)   // Check if a transmission event flag is set (indicating transmission completion)
  {
    lora_transmittedFlag = false;   // Reset the transmission flag for the next transmission cycle
    if (lora_transmissionState == RADIOLIB_ERR_NONE)   // If the previous transmission completed successfully
    {
      SX1262_INFO("transmission finished!");   // Log successful transmission message
    }
    else   // If the last transmission resulted in an error
    {
      SX1262_INFO("failed, code :%d", lora_transmissionState);   // Log failure with the corresponding error code
    }
    
    bsp_sx_radio->finishTransmit();   // Finalize the current transmission, ensuring the radio is ready for the next one
    
    sx1262_tx_counter++;   // Increment the transmission counter for each packet sent
    snprintf(text, sizeof(text), "TX_Hello World:%lu", (unsigned long)sx1262_tx_counter);   // Format a message with transmission count
    size_t tx_len = strlen(text);   // Calculate the length of the formatted text message
    lora_transmissionState = bsp_sx_radio->startTransmit((uint8_t *)text, tx_len + 1);   // Start transmitting the formatted text message (include null terminator)
    
    if (lora_transmissionState != RADIOLIB_ERR_NONE)   // Check if transmission start failed
    {
      SX1262_INFO("startTransmit failed, code: %d", lora_transmissionState);   // Log the error code for failed transmission start
    }

    return true;   // Return true indicating a transmission event was processed
  }
  return false;   // Return false if no transmission event occurred
}

/**
 * @brief Perform the SX1262 get transmit counter operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" uint32_t sx1262_get_tx_counter(void)
{
  return sx1262_tx_counter;
}

/**
 * @brief Initialize SX1262 transmit init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" esp_err_t sx1262_tx_init()
{
  BSP_SX1262 obj;
  esp_err_t err = ESP_OK;
  err = obj.Sx1262_tx_init();
  return err;
}

/**
 * @brief Initialize SX1262 transmit deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
extern "C" void sx1262_tx_deinit()
{
  BSP_SX1262 obj;
  obj.Sx1262_tx_deinit();
}

/**
 * @brief Apply the requested send lora pack radio operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" bool send_lora_pack_radio()
{
  BSP_SX1262 obj;
  return obj.Send_pack_radio();
}

/**
 * @brief Apply the requested set SX1262 receive flag operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Nothing.
 */
static void set_sx1262_rx_flag(void)
{
  lora_receivedFlag = true;
}

/**
 * @brief Initialize SX1262 receive init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
esp_err_t BSP_SX1262::Sx1262_rx_init()
{
  lora_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
  lora_hal.setSpiFrequency(8000000);
  lora_hal.spiBegin();

  bsp_sx_mod = new Module(&lora_hal, SX1262_GPIO_NSS, SX1262_GPIO_IRQ, SX1262_GPIO_NRST, SX1262_GPIO_BUSY);
  bsp_sx_radio = new SX1262(bsp_sx_mod);
  int state = begin_sx1262_radio(bsp_sx_radio, 3.3);
  if (state != RADIOLIB_ERR_NONE)
  {
    SX1262_ERROR("radio rx init failed, code :%d", state);
    return ESP_FAIL;
  }
  bsp_sx_radio->setPacketReceivedAction(set_sx1262_rx_flag);
  bsp_sx_radio->setRxBoostedGainMode(true);
  state = bsp_sx_radio->startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    SX1262_ERROR("radio start receive failed, code :%d", state);
    return ESP_FAIL;
  }
  return ESP_OK;
}

/**
 * @brief Initialize SX1262 receive deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
void BSP_SX1262::Sx1262_rx_deinit()
{
  lora_receivedFlag = false;
  bsp_sx_radio->clearPacketReceivedAction();
  bsp_sx_radio->standby();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  lora_hal.spiEnd();
}

/**
 * @brief Perform the received pack radio operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @param len Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
void BSP_SX1262::Received_pack_radio(size_t len)   // Function to process received LoRa data packets, parameter len indicates expected length
{
  if (lora_receivedFlag)   // Check if the receive flag is set (indicating data was received)
  {
    lora_receivedFlag = false;   // Reset the receive flag to avoid repeated processing
    
    // Get the actual received data length
    size_t actual_len = bsp_sx_radio->getPacketLength();   // Get packet length from SX1262 radio module
    if (actual_len > 0) {   // If a valid packet length is returned
      lora_received_len = actual_len;   // Use the actual received length
    } else {
      lora_received_len = len; //  Use the passed-in length as a fallback
    }
    
    uint8_t data[255];   // Define a buffer to store the received data
    int state = bsp_sx_radio->readData(data, lora_received_len);   // Read data from the SX1262 module into the buffer
    if (state == RADIOLIB_ERR_NONE)   // If reading succeeded
    {
      SX1262_INFO("Received packet!");   // Log message: packet successfully received
      SX1262_INFO("Valid Data : %.*s", lora_received_len, (char *)data);   // Print the received data as a string
      SX1262_INFO("RSSI:%.2f dBm", bsp_sx_radio->getRSSI());   // Log the received signal strength (RSSI)
      SX1262_INFO("SNR:%.2f dB", bsp_sx_radio->getSNR());   // Log the signal-to-noise ratio (SNR)
      SX1262_INFO("Frequency error:%.2f", bsp_sx_radio->getFrequencyError());   // Log the frequency error information
      
      //  Call the callback function to notify the upper application
      if (rx_data_callback != NULL) {   // If the callback function has been registered
        rx_data_callback((const char*)data, lora_received_len, bsp_sx_radio->getRSSI(), bsp_sx_radio->getSNR());   // Pass received data, length, RSSI, and SNR to the callback
      }
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)   // If CRC verification failed (data corrupted)
    {
      SX1262_ERROR("CRC error!");   // Log an error message indicating CRC mismatch
    }
    else   // Other unexpected errors during data reading
    {
      SX1262_ERROR("radio receive failed, code :%d", state);   // Log the specific error code
    }
  }
}

/**
 * @brief Initialize SX1262 receive init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" esp_err_t sx1262_rx_init()
{
  BSP_SX1262 obj;
  esp_err_t err = ESP_OK;
  err = obj.Sx1262_rx_init();
  return err;
}

/**
 * @brief Initialize SX1262 receive deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
extern "C" void sx1262_rx_deinit()
{
  BSP_SX1262 obj;
  obj.Sx1262_rx_deinit();
}

/**
 * @brief Perform the received lora pack radio operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @param len Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
extern "C" void received_lora_pack_radio(size_t len)
{
  BSP_SX1262 obj;
  obj.Received_pack_radio(len);
}

/**
 * @brief Handle the SX1262 set receive callback event and pass the result to the application.
 *
 * Called by the owning driver or framework when the event occurs.
 *
 * @param callback Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
extern "C" void sx1262_set_rx_callback(void (*callback)(const char* data, size_t len, float rssi, float snr))
{
  rx_data_callback = callback;
}

/**
 * @brief Perform the SX1262 get received len operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" size_t sx1262_get_received_len(void)
{
  return lora_received_len;
}

/**
 * @brief Perform the SX1262 is data received operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" bool sx1262_is_data_received(void)
{
  return lora_receivedFlag;
}

#endif
// --------------------------------------------------------------------------------

#if CONFIG_BSP_UART_TRANSPOND_ENABLED
/**
 * @brief Initialize UART transpond init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" esp_err_t uart_transpond_init()
{
  esp_err_t err = ESP_OK;
  const uart_config_t uart_config = {
      .baud_rate = 115200,                   // UART baud rate
      .data_bits = UART_DATA_8_BITS,         // UART byte size
      .parity = UART_PARITY_DISABLE,         // UART parity mode
      .stop_bits = UART_STOP_BITS_1,         // UART stop bits
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART HW flow control mode (cts/rts)
      .source_clk = UART_SCLK_DEFAULT,       // UART source clock selection
  };
  err = uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
  if (err != ESP_OK)
  {
    WIRELESS_UART_ERROR("uart driver install fail");
    return err;
  }
  uart_set_pin(UART_NUM_1, UART_GPIO_TXD, UART_GPIO_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  err = uart_param_config(UART_NUM_1, &uart_config);
  if (err != ESP_OK)
    return err;
  return err;
}

/**
 * @brief Initialize UART transpond deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
extern "C" void uart_transpond_deinit()
{
  uart_driver_delete(UART_NUM_1);
}
#endif



#if CONFIG_BSP_NRF2401_ENABLED
// static void set_nrf24_tx_flag(void)
// {
//   radio24_transmittedFlag = true;
// }

/**
 * @brief Initialize nRF24L01 transmit init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
esp_err_t BSP_NRF2401::NRF24_tx_init()
{
  nrf_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
  nrf_hal.setSpiFrequency(8000000);
  nrf_hal.spiBegin();

  bsp_nrf_mod = new Module(&nrf_hal, NRF24_GPIO_CS, NRF24_GPIO_IRQ, NRF24_GPIO_CE, -1);
  bsp_nrf_radio = new nRF24(bsp_nrf_mod);
  int state = begin_nrf24_radio(bsp_nrf_radio);
  if (state != RADIOLIB_ERR_NONE)
  {
    NRF2401_ERROR("radio tx init failed, code :%d", state);
    return ESP_FAIL;
  }
  uint8_t addr[] = {0x01, 0x02, 0x11, 0x12, 0xFF};
  state = bsp_nrf_radio->setTransmitPipe(addr);
  if (state != RADIOLIB_ERR_NONE)
  {
    NRF2401_ERROR("radio tx init failed, code :%d", state);
    return ESP_FAIL;
  }
  // bsp_nrf_radio->setPacketSentAction(set_nrf24_tx_flag);
  return ESP_OK;
}

/**
 * @brief Initialize nRF24L01 transmit deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
void BSP_NRF2401::NRF24_tx_deinit()
{
  // radio24_transmittedFlag = true;
  bsp_nrf_radio->finishTransmit();
  // bsp_nrf_radio->clearPacketSentAction();
  bsp_nrf_radio->standby();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  nrf_hal.spiEnd();
}

/**
 * @brief Apply the requested send pack radio operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
bool BSP_NRF2401::Send_pack_radio()
{

  // if (radio24_transmittedFlag)
  // {
  //   radio24_transmittedFlag = false;
  //   if (radio24_transmissionState == RADIOLIB_ERR_NONE)
  //   {
  //     NRF2401_INFO("transmission finished!");
  //   }
  //   else
  //   {
  //     NRF2401_INFO("failed, code :%d", radio24_transmissionState);
  //   }
  //   bsp_nrf_radio->finishTransmit();

  const uint8_t tx_buff[] = "#Hello World!";
  bsp_nrf_radio->transmit(tx_buff, 14, 0);
  NRF2401_INFO("transmission finished!");
  // radio24_transmissionState = bsp_nrf_radio->startTransmit(tx_buff, 14, 0);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  return true;
  // }
  // return false;
}

/**
 * @brief Initialize nRF24L01 transmit init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" esp_err_t nrf24_tx_init()
{
  BSP_NRF2401 obj;
  esp_err_t err = ESP_OK;
  err = obj.NRF24_tx_init();
  return err;
}

/**
 * @brief Initialize nRF24L01 transmit deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
extern "C" void nrf24_tx_deinit()
{
  BSP_NRF2401 obj;
  obj.NRF24_tx_deinit();
}

/**
 * @brief Apply the requested send nRF24L01 pack radio operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" bool send_nrf24_pack_radio()
{
  BSP_NRF2401 obj;
  return obj.Send_pack_radio();
}

/**
 * @brief Apply the requested set receive flag operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @return Nothing.
 */
static void set_rx_flag(void)
{
  radio24_receivedFlag = true;
}

/**
 * @brief Initialize nRF24L01 receive init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
esp_err_t BSP_NRF2401::NRF24_rx_init()
{
  nrf_hal.setSpiPins(RADIO_GPIO_CLK, RADIO_GPIO_MISO, RADIO_GPIO_MOSI);
  nrf_hal.setSpiFrequency(8000000);
  nrf_hal.spiBegin();

  bsp_nrf_mod = new Module(&nrf_hal, NRF24_GPIO_CS, NRF24_GPIO_IRQ, NRF24_GPIO_CE, -1);
  bsp_nrf_radio = new nRF24(bsp_nrf_mod);
  int state = begin_nrf24_radio(bsp_nrf_radio);
  if (state != RADIOLIB_ERR_NONE)
  {
    NRF2401_ERROR("radio rx init failed, code :%d", state);
    return ESP_FAIL;
  }
  uint8_t addr[] = {0x01, 0x02, 0x11, 0x12, 0xFF};
  state = bsp_nrf_radio->setReceivePipe(0, addr);
  if (state != RADIOLIB_ERR_NONE)
  {
    NRF2401_ERROR("radio rx init failed, code :%d", state);
    return ESP_FAIL;
  }
  bsp_nrf_radio->setPacketReceivedAction(set_rx_flag);
  state = bsp_nrf_radio->startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    NRF2401_ERROR("radio start receive failed, code :%d", state);
    return ESP_FAIL;
  }
  return ESP_OK;
}

/**
 * @brief Initialize nRF24L01 receive deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
void BSP_NRF2401::NRF24_rx_deinit()
{
  radio24_receivedFlag = false;
  bsp_nrf_radio->clearPacketReceivedAction();
  bsp_nrf_radio->standby();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  nrf_hal.spiEnd();
}

/**
 * @brief Perform the received pack radio operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @param len Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
void BSP_NRF2401::Received_pack_radio(size_t len)
{
  if (radio24_receivedFlag)
  {
    radio24_receivedFlag = false;
    uint8_t data[len];
    int state = bsp_nrf_radio->readData(data, len);
    if (state == RADIOLIB_ERR_NONE)
    {
      NRF2401_INFO("Received packet!");
      NRF2401_INFO("Valid Data : %.*s", len, (char *)data);
    }
    else
    {
      NRF2401_ERROR("radio receive failed, code :%d", state);
    }
    bsp_nrf_radio->startReceive();
  }
}

/**
 * @brief Initialize nRF24L01 receive init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Result produced by the operation; see the function body for its exact meaning.
 */
extern "C" esp_err_t nrf24_rx_init()
{
  BSP_NRF2401 obj;
  esp_err_t err = ESP_OK;
  err = obj.NRF24_rx_init();
  return err;
}

/**
 * @brief Initialize nRF24L01 receive deinit and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
extern "C" void nrf24_rx_deinit()
{
  BSP_NRF2401 obj;
  obj.NRF24_rx_deinit();
}

/**
 * @brief Perform the received nRF24L01 pack radio operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @param len Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
extern "C" void received_nrf24_pack_radio(size_t len)
{
  BSP_NRF2401 obj;
  obj.Received_pack_radio(len);
}
#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
