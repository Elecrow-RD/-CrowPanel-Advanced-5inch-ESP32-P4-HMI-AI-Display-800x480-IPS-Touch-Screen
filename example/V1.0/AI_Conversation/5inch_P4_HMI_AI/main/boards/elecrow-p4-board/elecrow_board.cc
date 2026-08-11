/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the elecrow_board responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#include "wifi_board.h"
#include "codecs/no_audio_codec.h"  // Use NoAudioCodec (direct I2S connection, no external codec)
#include "application.h"
#include "display/lcd_display.h"
#include "button.h"
#include "config.h"
#include <ssid_manager.h>

/*---------------------------------------------------------------
 * Board driver dependencies
 * Bring together the display, audio, input, camera, and network
 * interfaces used by the five-inch ESP32-P4 board implementation.
 *--------------------------------------------------------------*/
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_rgb.h"

#include <wifi_station.h>
#include <ssid_manager.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lvgl_port.h>
#include "esp_lcd_touch_gt911.h"  // If touch panel is supported
#include "p4_bsp_camera.h"
#include "bsp_stc8h1kxx.h"
#include "bsp_i2c.h"

#define TAG "ElecrowP4Board"

/*---------------------------------------------------------------
 * Optional I2C backlight adapter
 * Keep a Backlight-compatible adapter available for hardware
 * variants whose brightness controller is attached to an I2C bus.
 *--------------------------------------------------------------*/
class CustomBacklight : public Backlight {
public:
    /**
     * @brief Create an I2C-backed backlight adapter.
     *
     * @param i2c_handle Initialized I2C bus used by the controller.
     * @return A ready adapter object.
     *
     * Called only when constructing a board variant that uses an I2C
     * brightness controller. The current five-inch board uses STC8.
     */
    CustomBacklight(i2c_master_bus_handle_t i2c_handle)
        : Backlight(), i2c_handle_(i2c_handle) {}

protected:
    i2c_master_bus_handle_t i2c_handle_;

    /**
     * @brief Handle a requested brightness change for an I2C variant.
     *
     * @param brightness Requested level from 0 to 100.
     * @return Nothing.
     *
     * Called by the Backlight base class whenever application code changes
     * brightness. This placeholder records the request without touching the
     * five-inch board's STC8-controlled backlight.
     */
    virtual void SetBrightnessImpl(uint8_t brightness) override {
        ESP_LOGI(TAG, "Set backlight brightness to %u", brightness);
    }
};

/*---------------------------------------------------------------
 * STC8 backlight control
 * Coordinate the backlight power switch and PWM output so a zero
 * brightness request also removes panel backlight power.
 *--------------------------------------------------------------*/
class Stc8Backlight : public Backlight {
protected:
    /**
     * @brief Apply a brightness level through the STC8 controller.
     *
     * @param brightness Requested level from 0 to 100.
     * @return Nothing.
     *
     * Called by the Backlight base class during brightness restoration and
     * whenever the user or application changes the display brightness.
     */
    void SetBrightnessImpl(uint8_t brightness) override {
        // Power and PWM are updated together to avoid lighting the panel when
        // the requested duty cycle is zero.
        (void)stc8_gpio_set_level(STC8_GPIO_OUT_LCD_BL_POWER, brightness > 0 ? 1 : 0);
        // 5-inch demo: directly pass 0-100 to PWM
        (void)stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, brightness);
    }
};

class ElecrowP4Board : public WifiBoard {
private:
    // Owns the optional touch-controller I2C bus for the board lifetime.
    i2c_master_bus_handle_t touch_i2c_bus_;
    Button boot_button_;
    LcdDisplay *display_;
    Backlight *backlight_;
    P4BspCamera *camera_;

    /**
     * @brief Prepare the STC8 path that controls the audio amplifier.
     *
     * @param None.
     * @return Nothing. Failures are logged and leave audio output disabled.
     *
     * Called once from the board constructor before the audio codec is used.
     */
    void InitializeAudioCtrl() {
        // 5-inch demo: PA enable is controlled by STC8 extended IO (STC8_GPIO_OUT_AUDIO_SD, active-low: !state)
        // Initialize STC8 I2C here; the actual on/off is triggered in codec EnableOutput().
        esp_err_t err = i2c_init();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "BSP I2C init failed, audio amp control may not work: %s", esp_err_to_name(err));
            return;
        }
        err = stc8_i2c_init();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "STC8 I2C init failed, audio amp control may not work: %s", esp_err_to_name(err));
        } else {
            // Start muted so boot-time I2S activity cannot produce a pop.
            (void)stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD, 1);
            ESP_LOGI(TAG, "STC8 audio amp control initialized");
        }
    }

    /**
     * @brief Enable the camera sensor's dedicated 3.3 V supply.
     *
     * @param None.
     * @return ESP_OK on success, otherwise the LDO driver error.
     *
     * Called during board construction immediately before camera discovery.
     */
    static esp_err_t bsp_enable_camera_power(void) {
        static esp_ldo_channel_handle_t camera_pwr_chan = NULL;
        esp_ldo_channel_config_t ldo_cfg = {
            .chan_id = 4,  // LDO4
            .voltage_mv = 3300,  // 3.3V
        };
        esp_err_t err = esp_ldo_acquire_channel(&ldo_cfg, &camera_pwr_chan);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable camera power (LDO4): %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "Camera power enabled (LDO4: 3300mV)");
        return ESP_OK;
    }

    /**
     * @brief Create the 800 x 480, 16-bit RGB display pipeline.
     *
     * @param None.
     * @return Nothing. Fatal panel-driver failures stop initialization through
     *         ESP_ERROR_CHECK.
     *
     * Called once from the board constructor before restoring the backlight.
     */
    void InitializeLCD() {
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Initialize RGB panel: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
        // Assign fields individually because C++ designated initializers must
        // follow declaration order, which varies between ESP-IDF releases.
        esp_lcd_rgb_panel_config_t panel_conf = {};
        panel_conf.clk_src = LCD_CLK_SRC_DEFAULT;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
        panel_conf.psram_trans_align = 64;
#else
        panel_conf.dma_burst_size = 64;
#endif
        panel_conf.data_width = 16;
        panel_conf.bits_per_pixel = 16;
        panel_conf.de_gpio_num = RGB_PIN_NUM_DE;
        panel_conf.pclk_gpio_num = RGB_PIN_NUM_PCLK;
        panel_conf.vsync_gpio_num = RGB_PIN_NUM_VSYNC;
        panel_conf.hsync_gpio_num = RGB_PIN_NUM_HSYNC;
        panel_conf.disp_gpio_num = RGB_PIN_NUM_DISP_EN;
        panel_conf.data_gpio_nums[0]  = RGB_PIN_NUM_DATA0;
        panel_conf.data_gpio_nums[1]  = RGB_PIN_NUM_DATA1;
        panel_conf.data_gpio_nums[2]  = RGB_PIN_NUM_DATA2;
        panel_conf.data_gpio_nums[3]  = RGB_PIN_NUM_DATA3;
        panel_conf.data_gpio_nums[4]  = RGB_PIN_NUM_DATA4;
        panel_conf.data_gpio_nums[5]  = RGB_PIN_NUM_DATA5;
        panel_conf.data_gpio_nums[6]  = RGB_PIN_NUM_DATA6;
        panel_conf.data_gpio_nums[7]  = RGB_PIN_NUM_DATA7;
        panel_conf.data_gpio_nums[8]  = RGB_PIN_NUM_DATA8;
        panel_conf.data_gpio_nums[9]  = RGB_PIN_NUM_DATA9;
        panel_conf.data_gpio_nums[10] = RGB_PIN_NUM_DATA10;
        panel_conf.data_gpio_nums[11] = RGB_PIN_NUM_DATA11;
        panel_conf.data_gpio_nums[12] = RGB_PIN_NUM_DATA12;
        panel_conf.data_gpio_nums[13] = RGB_PIN_NUM_DATA13;
        panel_conf.data_gpio_nums[14] = RGB_PIN_NUM_DATA14;
        panel_conf.data_gpio_nums[15] = RGB_PIN_NUM_DATA15;

        panel_conf.timings.pclk_hz = RGB_LCD_PIXEL_CLOCK_HZ;
        panel_conf.timings.h_res = DISPLAY_WIDTH;
        panel_conf.timings.v_res = DISPLAY_HEIGHT;
        panel_conf.timings.hsync_pulse_width = RGB_LCD_HSYNC;
        panel_conf.timings.hsync_back_porch = RGB_LCD_HBP;
        panel_conf.timings.hsync_front_porch = RGB_LCD_HFP;
        panel_conf.timings.vsync_pulse_width = RGB_LCD_VSYNC;
        panel_conf.timings.vsync_back_porch = RGB_LCD_VBP;
        panel_conf.timings.vsync_front_porch = RGB_LCD_VFP;
        panel_conf.timings.flags.hsync_idle_low = false;
        panel_conf.timings.flags.vsync_idle_low = false;
        panel_conf.timings.flags.de_idle_high = false;
        panel_conf.timings.flags.pclk_active_neg = true;
        panel_conf.timings.flags.pclk_idle_high = true;

        panel_conf.flags.fb_in_psram = 1;
        panel_conf.num_fbs = 2;

        ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_conf, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

        // An RGB panel has no command panel-I/O object; the LVGL adapter sends
        // pixels through the RGB peripheral and keeps its buffers in PSRAM.
        display_ = new RgbLcdDisplay(nullptr, panel,
                                     DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                     DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                     DISPLAY_SWAP_XY,
                                     /*swap_bytes*/ true,
                                     /*buff_spiram*/ true,
                                     /*buffer_size*/ (uint32_t)DISPLAY_WIDTH * (uint32_t)DISPLAY_HEIGHT,
                                     /*double_buffer*/ false);

        backlight_ = new Stc8Backlight();

        ESP_LOGI(TAG, "RGB LCD initialized: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    /**
     * @brief Create the optional GT911 touch-controller I2C bus.
     *
     * @param None.
     * @return Nothing. Driver failures stop initialization through
     *         ESP_ERROR_CHECK.
     *
     * Called from the constructor only when touch support is enabled.
     */
    void InitializeTouchI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = TOUCH_I2C_PORT,
            .sda_io_num = TOUCH_I2C_SDA_PIN,
            .scl_io_num = TOUCH_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &touch_i2c_bus_));
        ESP_LOGI(TAG, "Touch I2C bus initialized: SDA=%d, SCL=%d", TOUCH_I2C_SDA_PIN, TOUCH_I2C_SCL_PIN);
    }

    /**
     * @brief Attach the GT911 touch controller to the default LVGL display.
     *
     * @param None.
     * @return Nothing. The function returns early when no I2C bus exists.
     *
     * Called after InitializeTouchI2c() when touch support is enabled.
     */
    void InitializeTouch() {
        // If I2C is not initialized, skip touch initialization
        if (touch_i2c_bus_ == nullptr) {
            ESP_LOGW(TAG, "Touch I2C bus not initialized, skipping touch panel");
            return;
        }
        
        esp_lcd_touch_handle_t tp;
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH,
            .y_max = DISPLAY_HEIGHT,
            .rst_gpio_num = TOUCH_GPIO_RST,
            .int_gpio_num = TOUCH_GPIO_INT,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        };
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        // Build the configuration explicitly because some GT911 component
        // versions expand their helper macro in an order rejected by C++.
        esp_lcd_panel_io_i2c_config_t tp_io_config = {};
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
        tp_io_config.control_phase_bytes = 1;
        tp_io_config.dc_bit_offset = 0;
        tp_io_config.lcd_cmd_bits = 16;
        tp_io_config.flags.disable_control_phase = 1;
        tp_io_config.scl_speed_hz = 100000;
        tp_io_config.scl_speed_hz = 400 * 1000;  // 400kHz I2C speed (aligned with bsp_display.c)
        // GT911 modules can boot at either supported address, so retry with
        // the backup address only when the primary address is unavailable.
        esp_err_t err = esp_lcd_new_panel_io_i2c(touch_i2c_bus_, &tp_io_config, &tp_io_handle);
        if (err != ESP_OK) {
            // If primary address fails, try the alternate address
            tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
            ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(touch_i2c_bus_, &tp_io_config, &tp_io_handle));
        }
        ESP_LOGI(TAG, "Initialize touch controller GT911");
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp = lv_display_get_default(),
            .handle = tp,
        };
        lvgl_port_add_touch(&touch_cfg);
        ESP_LOGI(TAG, "Touch panel initialized successfully: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    /**
     * @brief Register the function-button behavior.
     *
     * @param None.
     * @return Nothing.
     *
     * Called once during board construction after display initialization.
     */
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && 
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
        ESP_LOGI(TAG, "Buttons initialized");
    }

    /**
     * @brief Construct the board camera adapter.
     *
     * @param None.
     * @return Nothing. Construction failures leave camera_ set to nullptr.
     *
     * Called after the camera supply has been enabled during board startup.
     */
    void InitializeCamera() {
        try {
            camera_ = new P4BspCamera();
            ESP_LOGI(TAG, "Camera initialized successfully");
        } catch (...) {
            ESP_LOGE(TAG, "Failed to initialize camera");
            camera_ = nullptr;
        }
    }

public:
    /**
     * @brief Initialize all board-specific hardware in dependency order.
     *
     * @param None.
     * @return A fully initialized board object.
     *
     * Called once by the board factory during application startup.
     */
    ElecrowP4Board() : touch_i2c_bus_(nullptr), boot_button_(BOOT_BUTTON_GPIO), camera_(nullptr) {
        ESP_LOGI(TAG, "Initializing Elecrow P4 Board...");
        
        // Install the shared GPIO interrupt service before any peripheral can
        // register an interrupt handler. An existing service is also valid.
        esp_err_t err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(err));
        }
        
        InitializeAudioCtrl();
        InitializeLCD();
        //InitializeTouchI2c();  // Initialize touch panel I2C bus
        //InitializeTouch();     // Initialize touch panel (if I2C is configured)
        InitializeButtons();
        // Restore brightness only after the RGB panel can safely receive data.
        GetBacklight()->RestoreBrightness();
        bsp_enable_camera_power();
        InitializeCamera();
        ESP_LOGI(TAG, "Elecrow P4 Board initialized successfully");
    }
    
    /**
     * @brief Return the singleton audio interface for this board.
     *
     * @param None.
     * @return Pointer to the board-lifetime audio codec adapter.
     *
     * Called by the audio service when capture or playback is first needed.
     */
    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplexPdm audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,   // Microphone sample rate: 16000
            AUDIO_OUTPUT_SAMPLE_RATE,  // Speaker sample rate: 16000
            AUDIO_I2S_GPIO_BCLK,       // Speaker BCLK pin
            AUDIO_I2S_GPIO_WS,         // Speaker WS/LRCLK pin
            AUDIO_I2S_GPIO_DOUT,       // Speaker DOUT pin
            AUDIO_PDM_MIC_CLK,         // PDM microphone clock pin
            AUDIO_PDM_MIC_DIN);        // PDM microphone data input pin
        
        // Install the hook once. AUDIO_SD is active-low, so enabling playback
        // writes zero and disabling playback returns the amplifier to mute.
        static bool hook_inited = false;
        if (!hook_inited) {
            audio_codec.SetOutputEnableHook([](bool enable, void* /*ctx*/) {
                (void)stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD, enable ? 0 : 1);
            }, nullptr);
            hook_inited = true;
        }
        return &audio_codec;
    }

    /**
     * @brief Provide the board display to the application.
     * @param None.
     * @return Pointer to the initialized RGB display.
     * @note Called whenever UI services need the active display.
     */
    virtual Display* GetDisplay() override {
        return display_;
    }

    /**
     * @brief Provide a usable backlight controller.
     * @param None.
     * @return The STC8 controller, or a fallback PWM controller if absent.
     * @note Called during startup and on later brightness changes.
     */
    virtual Backlight* GetBacklight() override {
        if (backlight_ != nullptr) {
            return backlight_;
        }
        static PwmBacklight fallback(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT, 30000);
        return &fallback;
    }

    /**
     * @brief Provide the camera interface to vision features.
     * @param None.
     * @return Camera pointer, or nullptr if camera initialization failed.
     * @note Called when preview or image explanation is requested.
     */
    virtual Camera* GetCamera() override {
        return camera_;
    }

    /**
     * @brief Prepare saved Wi-Fi credentials and start networking.
     *
     * @param None.
     * @return Nothing.
     *
     * Called by the application after board hardware initialization. The
     * delay gives the ESP32-C6 coprocessor time to establish its SDIO link.
     */
    virtual void StartNetwork() override {
        ESP_LOGI(TAG, "ElecrowP4Board::StartNetwork() called");
        
        ESP_LOGI(TAG, "Waiting for ESP32-C6 coprocessor to be ready...");
        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds for C6 to boot and initialize SDIO
        
        // Preserve user networks while ensuring the course's default network
        // exists and receives the current password from config.h.
        auto& ssid_manager = SsidManager::GetInstance();
        auto ssid_list = ssid_manager.GetSsidList();
        
        ESP_LOGI(TAG, "Current WiFi SSID list size: %d", ssid_list.size());
        for (size_t i = 0; i < ssid_list.size(); i++) {
            ESP_LOGI(TAG, "  [%d] SSID: %s", i, ssid_list[i].ssid.c_str());
        }
        
        // If no configuration exists, add the default Wi-Fi
        if (ssid_list.empty()) {
            ESP_LOGI(TAG, "No WiFi configured, adding default WiFi: %s", DEFAULT_WIFI_SSID);
            ssid_manager.AddSsid(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
            ESP_LOGI(TAG, "Default WiFi added successfully");
        } else {
            // Check if the configured default Wi-Fi already exists; if not, add it
            bool found = false;
            for (const auto& item : ssid_list) {
                if (item.ssid == DEFAULT_WIFI_SSID) {
                    found = true;
                    ESP_LOGI(TAG, "WiFi '%s' already configured, updating password from macros", DEFAULT_WIFI_SSID);
                    ssid_manager.AddSsid(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
                    break;
                }
            }
            if (!found) {
                ESP_LOGI(TAG, "Adding default WiFi: %s", DEFAULT_WIFI_SSID);
                ssid_manager.AddSsid(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
                ESP_LOGI(TAG, "Default WiFi added successfully");
            }
        }
        
        // Credential preparation is board-specific; connection state handling
        // remains in the shared WifiBoard implementation.
        WifiBoard::StartNetwork();
    }
};

/*---------------------------------------------------------------
 * Board registration
 * Expose this implementation to the common board factory selected
 * by the project's build configuration.
 *--------------------------------------------------------------*/
DECLARE_BOARD(ElecrowP4Board);
