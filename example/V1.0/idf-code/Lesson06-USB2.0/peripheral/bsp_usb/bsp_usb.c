/**
 * @file bsp_usb.c
 * @brief Teaching source for 5inch_P4_IDF_06_USB2_0_HID_Mouse.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_usb.h"   // Include the custom USB BSP header
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

static const char *TAG = "USB_HID";   // Logging tag for this file

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)  
// Total length of configuration descriptor (base length + HID length)

/**
 * @brief HID report descriptor
 *
 * In this example we implement Keyboard + Mouse HID device,
 * so we must define both report descriptors
 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))  
    // HID mouse report descriptor with mouse protocol ID
};

/**
 * @brief String descriptor
 */
const char* hid_string_descriptor[5] = {
    (char[]){0x09, 0x04},  // 0: Supported language = English (0x0409)
    "Espressif",           // 1: Manufacturer string
    "Advance-P4 HID Mouse",// 2: Product name string
    "123456",              // 3: Serial number string
    "HID Mouse Interface", // 4: HID interface description string
};

/**
 * @brief Configuration descriptor - only includes mouse function
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Config number, interface count, string index, total length, attributes, power (mA)
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, boot protocol, report descriptor length, endpoint, size, polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/


/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application returns pointer to descriptor; must exist until transfer completes
/**
 * @brief Perform the tud hid descriptor report cb operation.
 *
 * Called by the application when this module operation is required.
 * @param instance Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // Only one interface/report is used, so 'instance' is ignored
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer with report data and return length
// Returning 0 will stall the request
/**
 * @brief Perform the tud hid get report cb operation.
 *
 * Called by the application when this module operation is required.
 * @param instance Input or output value used by this operation.
 * @param report_id Input or output value used by this operation.
 * @param report_type Input or output value used by this operation.
 * @param buffer Input or output value used by this operation.
 * @param reqlen Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    return 0;  // No report data provided in this example
}

// Invoked when received SET_REPORT control request
// Or when OUT endpoint receives data
/**
 * @brief Perform the tud hid set report cb operation.
 *
 * Called by the application when this module operation is required.
 * @param instance Input or output value used by this operation.
 * @param report_id Input or output value used by this operation.
 * @param report_type Input or output value used by this operation.
 * @param buffer Input or output value used by this operation.
 * @param bufsize Input or output value used by this operation.
 * @return None.
 */
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // Not implemented in this example
}

/********* Application ***************/

// Send mouse movement deltas over HID
/**
 * @brief Perform the send hid mouse delta operation.
 *
 * Called by the application when this module operation is required.
 * @param delta_x Input or output value used by this operation.
 * @param delta_y Input or output value used by this operation.
 * @return None.
 */
void send_hid_mouse_delta(int8_t delta_x, int8_t delta_y)
{
    if (tud_hid_ready()) {  
        // Report mouse movement: report_id=HID mouse, button=0x00, X delta, Y delta, no wheel, no pan
        tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, 0x00, delta_x, delta_y, 0, 0);
    }
}

// Check if USB HID device is ready
/**
 * @brief Perform the is usb ready operation.
 *
 * Called by the application when this module operation is required.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
bool is_usb_ready(void)
{
    return tud_hid_ready();
}

// Initialize USB HID device
/**
 * @brief Perform the usb init operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t usb_init(void)
{
    ESP_LOGI(TAG, "Initializing USB HID Mouse");  // Log initialization start

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,   // Use default device descriptor
        .string_descriptor = hid_string_descriptor, // Assign string descriptors
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]), // Number of strings
        .external_phy = false,       // Use internal PHY
        .fs_configuration_descriptor = hid_configuration_descriptor, // FS config descriptor
        .hs_configuration_descriptor = hid_configuration_descriptor, // HS config descriptor (same as FS here)
        .qualifier_descriptor = NULL, // No qualifier descriptor
    };

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);  // Install TinyUSB driver
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(err)); // Log failure
        return err;
    }
    
    ESP_LOGI(TAG, "USB HID Mouse initialization completed"); // Log success
    return ESP_OK;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
