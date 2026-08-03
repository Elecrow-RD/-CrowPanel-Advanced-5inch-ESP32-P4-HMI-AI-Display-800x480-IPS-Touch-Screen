/**
 * @file board_config.h
 * @brief Defines the hardware and library configuration used by the lesson.
 *
 * This file belongs to the Lesson11-Playback_After_Recording course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Board Config Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

#pragma once

/*********************** Pin define ***********************/
// GPIO pins for I2C, has touch chip GT911,STC8H1KXX
#define I2C_GPIO_SCL            (46)    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA            (45)    // GPIO number used for I2C SDA (data) line

#define AUDIO_GPIO_CTRL         (-1)    // GPIO pin number for audio power
#define AUDIO_POWER_ENABLE      (LOW)   // GPIO set level to enable audio power
#define AUDIO_POWER_DISABLE     (HIGH)  // GPIO set level to disable audio power

#define AUDIO_GPIO_LRCLK        (21)    // GPIO pin number for LRCLK (Left-Right Clock  of I2S)
#define AUDIO_GPIO_BCLK         (22)    // GPIO pin number for BCLK (Bit Clock of I2S)
#define AUDIO_GPIO_SDATA        (23)    // GPIO pin number for SDATA (Serial Data of I2S)

#define MIC_GPIO_CLK            (24)    // GPIO pin number for microphone BCLK (Bit Clock of PDM)
#define MIC_GPIO_SDIN           (25)    // GPIO pin number for microphone SDIN (Serial Data of PDM)
/*********************** Pin define ***********************/
