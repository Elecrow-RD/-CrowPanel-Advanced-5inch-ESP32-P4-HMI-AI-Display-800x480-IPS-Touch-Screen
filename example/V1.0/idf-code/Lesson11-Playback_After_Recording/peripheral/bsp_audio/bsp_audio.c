/**
 * @file bsp_audio.c
 * @brief Teaching source for 5inch_P4_IDF_11_Playback_After_Recording.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_audio.h"

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
i2s_chan_handle_t tx_chan;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

esp_err_t audio_init()
{
    esp_err_t err = ESP_OK;

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_1,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 256,
        .auto_clear = true,
        .intr_priority = 0,
    };
    err = i2s_new_channel(&chan_cfg, &tx_chan, NULL);
    if (err != ESP_OK)
        return err;
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 16000,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AUDIO_GPIO_BCLK,
            .ws = AUDIO_GPIO_LRCLK,
            .dout = AUDIO_GPIO_SDATA,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK)
        return err;
    err = i2s_channel_enable(tx_chan);
    if (err != ESP_OK)
        return err;
    return err;
}

/**
 * @brief Perform the audio ctrl init operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t audio_ctrl_init()
{
    return ESP_OK;
}

/**
 * @brief Perform the set Audio ctrl operation.
 *
 * Called by the application when this module operation is required.
 * @param state Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t set_Audio_ctrl(bool state)
{
    stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD, !state);
    return ESP_OK;
}

/**
 * @brief Perform the get audio handle operation.
 *
 * Called by the application when this module operation is required.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
i2s_chan_handle_t get_audio_handle()
{
    return tx_chan;
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
