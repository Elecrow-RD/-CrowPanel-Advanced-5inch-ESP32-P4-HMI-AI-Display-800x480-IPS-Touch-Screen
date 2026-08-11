/*---------------------------------------------------------------
 * Teaching module overview: Audio processing
 * This file groups the no_audio_codec responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef _NO_AUDIO_CODEC_H
#define _NO_AUDIO_CODEC_H

#include "audio_codec.h"

#include <driver/gpio.h>
#include <driver/i2s_pdm.h>
#include <mutex>

class NoAudioCodec : public AudioCodec {
protected:
    std::mutex data_if_mutex_;
    gpio_num_t pa_pin_ = GPIO_NUM_NC;
    bool pa_inverted_ = false;
    void (*output_enable_hook_)(bool enable, void* ctx) = nullptr;
    void* output_enable_hook_ctx_ = nullptr;

    virtual int Write(const int16_t* data, int samples) override;
    virtual int Read(int16_t* dest, int samples) override;

public:
    // 可选：用于像 demo 的 set_Audio_ctrl() 那样控制功放/静音脚
    // - pin: GPIO_NUM_NC 表示不使用
    // - inverted: true 表示输出电平取反（等价于 demo 里的 `!state`）
    void SetPaPin(gpio_num_t pin, bool inverted = false);
    // 可选：输出使能回调（用于 STC8/IO扩展等非GPIO控制）
    void SetOutputEnableHook(void (*hook)(bool enable, void* ctx), void* ctx);
    void EnableOutput(bool enable) override;

    virtual ~NoAudioCodec();
};

class NoAudioCodecDuplex : public NoAudioCodec {
public:
    NoAudioCodecDuplex(int input_sample_rate, int output_sample_rate, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);
};

class NoAudioCodecSimplex : public NoAudioCodec {
public:
    NoAudioCodecSimplex(int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk, gpio_num_t spk_ws, gpio_num_t spk_dout, gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din);
    NoAudioCodecSimplex(int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk, gpio_num_t spk_ws, gpio_num_t spk_dout, i2s_std_slot_mask_t spk_slot_mask, gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din, i2s_std_slot_mask_t mic_slot_mask);
};

class NoAudioCodecSimplexPdm : public NoAudioCodec {
public:
    NoAudioCodecSimplexPdm(int input_sample_rate, int output_sample_rate, gpio_num_t spk_bclk, gpio_num_t spk_ws, gpio_num_t spk_dout, gpio_num_t mic_sck,  gpio_num_t mic_din);
    int Read(int16_t* dest, int samples);
};

#endif // _NO_AUDIO_CODEC_H
