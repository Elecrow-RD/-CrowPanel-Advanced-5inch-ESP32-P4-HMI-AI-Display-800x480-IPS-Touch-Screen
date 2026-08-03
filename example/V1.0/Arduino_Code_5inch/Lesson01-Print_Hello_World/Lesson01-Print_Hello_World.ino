/**
 * @file Lesson01-Print_Hello_World.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson01-Print_Hello_World course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson01-Print Hello World Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

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
    Serial.begin(115200);  // Init Uart
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
    static int i = 0;
    Serial.printf("Hello world: %d\n", i++);   //print "Hello World!"
    delay(1000);    // Delay 1 second
}
