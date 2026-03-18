#pragma once

/*********************** Pin define ***********************/
// GPIO pins for GT911 touch panel
#define Touch_GPIO_RST      (36)    // Reset pin
#define Touch_GPIO_INT      (42)    // Interrupt pin

// GPIO pins for I2C, has touch chip GT911
#define I2C_GPIO_SCL        (46)    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA        (45)    // GPIO number used for I2C SDA (data) line

// panel parameters
#define H_size              (800)   // Horizontal resolution (X-axis)
#define V_size              (480)   // Vertical resolution (Y-axis)
/*********************** Pin define ***********************/
