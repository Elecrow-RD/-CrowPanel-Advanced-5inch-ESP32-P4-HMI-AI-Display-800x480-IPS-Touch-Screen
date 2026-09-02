/**
 * @file main.c
 * @brief Teaching source for 5inch_P4_IDF_12_SD_Music_Playback.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
void init_fail(const char *name, esp_err_t err)
{
    static bool state = false;
    while (1)
    {
        if (!state)
        {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));
            state = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Perform the Init operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return None.
 */
void Init(void)
{
    static esp_err_t err = ESP_OK;

    err = i2c_init();
    if (err != ESP_OK)
        init_fail("i2c", err);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    err = stc8_i2c_init();
    if (err != ESP_OK)
        init_fail("stc8h1kxx", err);
    MAIN_INFO("I2C and stc8 init success");  // Print success log

    err = sd_init(); /*SD Initialization*/
    if (err != ESP_OK)
        init_fail("sd", err);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    err = audio_ctrl_init(); /*Audio CTRL Initialization*/
    if (err != ESP_OK)
        init_fail("audio ctrl", err);
        
    set_Audio_ctrl(false);
    err = audio_init(); /*Audio Initialization*/
    if (err != ESP_OK)
        init_fail("audio", err);
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

/**
 * @brief Start the lesson application.
 *
 * Called once by ESP-IDF after the system startup sequence completes.
 * @return None.
 */
void app_main(void)
{
    MAIN_INFO("----------Start the test--------");
    Init();

    Audio_play_wav_sd("/sdcard/huahai.wav"); /*Play the WAV file stored on the SD card that was recorded by the microphone*/
}
/* ——————————————————————————————————————— Functional function end ————————————————————————————————————————— */
