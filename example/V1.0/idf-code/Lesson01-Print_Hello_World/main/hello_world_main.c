/**
 * @file hello_world_main.c
 * @brief Teaching source for 5inch_P4_IDF_01_Print_Hello_World.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Start the lesson application.
 *
 * Called once by ESP-IDF after the system startup sequence completes.
 * @return None.
 */
void app_main(void)
{
    int i = 0;
    while (1) {
        printf("Hello world: %d\n", i++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay 1 second
    }
}
