#include <Arduino.h>
#include <IRremote.h>

// Pin definition
const int switchPin = 7;

// Define a variable for the button state
int buttonState = 0;

// Create IR Send Object
IRsend irsend;

void setup(){

// Set Switch pin as Input
pinMode(switchPin, INPUT);
}
void loop()
{
// Set button state depending upon switch position
buttonState = digitalRead(switchPin);

// If button is pressed send trigger LED hex over IR
if (buttonState == HIGH){

// Send LED hex
irsend.sendNEC(0xFEA857, 32);

}

// Add a small delay before repeating
delay(200);
}


void loop()

// check if LED should be turned on
if (shouldTurnLedOn)

// Send HIGH signal to LED_PIN to turn on the LED
digitalWrite(LED_PIN, HIGH);

else
{

// Send LOW signal to LED_PIN to turn off the LED
digitalWrite(LED_PIN, LOW);

// Decode the received results (if there are any)
if (irrecv.decode(&results))

// Send the result to our serial monitor
Serial.println(results.value, HEX);

// If the result is the expected hex value, trigger LED state
if (results.value == expectedHex)

// Trigger LED state
shouldTurnLedOn = !shouldTurnLedOn;

// Continue receiving IR messages
irrecv.resume();

}
