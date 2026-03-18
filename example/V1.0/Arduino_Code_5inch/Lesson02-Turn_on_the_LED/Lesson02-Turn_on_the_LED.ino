/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "board_config.h"
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

void setup() {
  // put your setup code here, to run once:

  // Initialize GPIO
  pinMode(PIN_LED, OUTPUT);
    
}

void loop() {
  // put your main code here, to run repeatedly:

  // LED is on
  digitalWrite(PIN_LED, HIGH); // Set GPIO48 output to high (1) or low (0)
  delay(1000);


  // LED is off
  digitalWrite(PIN_LED, LOW);  // Set GPIO48 output to high (1) or low (0)
  delay(1000);
}
