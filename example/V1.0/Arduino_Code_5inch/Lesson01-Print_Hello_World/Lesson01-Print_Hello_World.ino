/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);  // Init Uart
}

void loop() {
    // put your main code here, to run repeatedly:
    static int i = 0;
    Serial.printf("Hello world: %d\n", i++);   //print "Hello World!"
    delay(1000);    // Delay 1 second
}
