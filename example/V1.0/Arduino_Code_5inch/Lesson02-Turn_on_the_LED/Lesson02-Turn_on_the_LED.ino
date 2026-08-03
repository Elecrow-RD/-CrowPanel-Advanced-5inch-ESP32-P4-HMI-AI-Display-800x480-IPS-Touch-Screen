/**
 * @file Lesson02-Turn_on_the_LED.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson02-Turn_on_the_LED course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson02-Turn On The Led Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "board_config.h"
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

/**
 * @brief Initialize the hardware and services required by this lesson.
 *
 * Called once by the Arduino runtime after power-up or reset.
 *
 * @return Nothing.
 */
void setup() {
  // put your setup code here, to run once:

  // Initialize GPIO
  pinMode(PIN_LED, OUTPUT);
    
}

/**
 * @brief Run the lesson's recurring application work.
 *
 * Called repeatedly by the Arduino runtime after setup completes.
 *
 * @return Nothing.
 */
void loop() {
  // put your main code here, to run repeatedly:

  // LED is on
  digitalWrite(PIN_LED, HIGH); // Set GPIO48 output to high (1) or low (0)
  delay(1000);


  // LED is off
  digitalWrite(PIN_LED, LOW);  // Set GPIO48 output to high (1) or low (0)
  delay(1000);
}
