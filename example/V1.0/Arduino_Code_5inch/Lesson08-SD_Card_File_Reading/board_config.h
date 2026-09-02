/**
 * @file board_config.h
 * @brief Defines the hardware and library configuration used by the lesson.
 *
 * This file belongs to the Lesson08-SD_Card_File_Reading course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Board Config Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

#pragma once

/*********************** Pin define ***********************/
// SD card GPIO with SD_MMC
#define SD_GPIO_MMC_CLK     (43)
#define SD_GPIO_MMC_CMD     (44)
#define SD_GPIO_MMC_D0      (39)
// SD card GPIO with SPI
#define SD_GPIO_SPI_CLK     SD_GPIO_MMC_CLK
#define SD_GPIO_SPI_MOSI    SD_GPIO_MMC_CMD
#define SD_GPIO_SPI_MISO    SD_GPIO_MMC_D0
/*********************** Pin define ***********************/
