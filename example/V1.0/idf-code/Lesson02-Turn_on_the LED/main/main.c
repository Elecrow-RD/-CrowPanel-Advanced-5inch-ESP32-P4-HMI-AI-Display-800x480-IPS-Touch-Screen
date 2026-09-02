/**
 * @file main.c
 * @brief Teaching source for 5inch_P4_IDF_02_Turn_On_LED.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_extra.h"

/**
 * @brief Perform the led blink task operation.
 *
 * Called by the FreeRTOS scheduler after the task is created.
 * @param pvParameters Input or output value used by this operation.
 * @return None.
 */
void led_blink_task(void *pvParameters)
{
    // Initialize GPIO
    gpio_extra_init();

    while (1)
    {
        // LED is on
        gpio_extra_set_level(1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // LED is off
        gpio_extra_set_level(0);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // delay 1 second
    }
}

/**
 * @brief Start the lesson application.
 *
 * Called once by ESP-IDF after the system startup sequence completes.
 * @return None.
 */
void app_main(void)
{
    xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 5, NULL);
}
