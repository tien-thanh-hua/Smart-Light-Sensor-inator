#include <Adafruit_LiquidCrystal.h>

const int LIGHT_RELAY_PIN = 12;

const int AL_LED = 2;
const int US_LED = 3;
const int PIR_LED = 4;

const int BUTTON_PIN = 5;
const int TEMP_PIN = A0;

const int PHOTORESISTOR_PIN = A1;
const int PIR_PIN = 8;

Adafruit_LiquidCrystal lcd_1(0);

int reading;
float celsius;
float relativeHumidity = 0;
float decibel = 0;
float cm;
boolean isPIRDetected;

boolean isLightOn = false;

int buttonPushCounter = 1;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

float readUltrasonicDistance(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT);  // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH);
}

 
void setup()	
{
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  
  pinMode(AL_LED, OUTPUT);
  pinMode(US_LED, OUTPUT);
  pinMode(PIR_LED, OUTPUT);
  
  pinMode(TEMP_PIN, INPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  
  // initialize the button pin as a input:
  pinMode(BUTTON_PIN, INPUT);
  
  // initialize serial communication:
  Serial.begin(9600);
  lcd_1.begin(16, 2); //sets up number of columns (16) and rows (2)
  lcd_1.setBacklight(1);
}

void loop()
{
  isPIRDetected = digitalRead(PIR_PIN);
  sensorLED(PIR_LED, isPIRDetected);
  
  // measure the ping time in cm
  cm = 0.01723 * readUltrasonicDistance(7, 7);
  boolean isUltrasonicDetected = (cm <= 335);
  sensorLED(US_LED, isUltrasonicDetected);
  
  int light = analogRead(PHOTORESISTOR_PIN);
  boolean isLightDetected = (light <= 512);
  sensorLED(AL_LED, isLightDetected);
  
  reading = analogRead(TEMP_PIN);
  float volt = (reading/1023.0) * 5;
  celsius = (volt - 0.5) * 100;
    
  if (isPIRDetected && isUltrasonicDetected && isLightDetected) {
    digitalWrite(LIGHT_RELAY_PIN, HIGH);
   	isLightOn = true;
  } else if (isUltrasonicDetected && isLightOn) {
  	digitalWrite(LIGHT_RELAY_PIN, HIGH);
  } else {
  	digitalWrite(LIGHT_RELAY_PIN, LOW);
    isLightOn = false;
  }
  
  lcdButton();
  
  //Serial.print(cm);
  //Serial.println("cm");
  //Serial.print(celsius);
  //Serial.println("C");
  //Serial.println(light);
  //Serial.println("cd");
  delay(38); // Wait for 100 millisecond(s)
}

void lcdButton() {
  // read the pushbutton input pin:
  buttonState = digitalRead(BUTTON_PIN);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      buttonPushCounter++;
      clearLCDFirstLine();
      Serial.println("Button pushed");
      switch (buttonPushCounter) {
    	case 1:
      		Serial.println("Temperature Mode");
      		break;
      	case 2:
      		Serial.println("Humidity Mode");
      		break;
      	case 3:
      		Serial.println("Loudness Mode");
      		break;
      }
    } else {
      // if the current state is LOW then the button went from on to off:
      Serial.println("Button released");
    }
    
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;


  // turns on the LED every four button pushes by checking the modulo of the
  // button push counter. the modulo function gives you the remainder of the
  // division of two numbers:
  switch (buttonPushCounter) {
  	case 1:
    	lcdTemp();
    	break;
    case 2:
    	lcdHumidity();
    	break;
    case 3:
		lcdLoudness();
    	// resets counter, starting new cycle
    	buttonPushCounter = 0;
    	break;
	}
}

void sensorLED(int triggerPin, boolean value) {
  if (value == true) {
  	digitalWrite(triggerPin, HIGH);
  } else {
  	digitalWrite(triggerPin, LOW);
  }
}

void clearLCDFirstLine() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("                ");
}

void lcdTemp() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Temperature:");
  lcd_1.setCursor(0, 1);
  // Extra spaces prevent redundant " C" 
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: -110.46 C -> -75.02 CC -> 1.24 CCCC)
  lcd_1.setCursor(0, 1);
  lcd_1.print(celsius);
  lcd_1.print("C       ");
}

void lcdHumidity() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Humidity:");
  lcd_1.setCursor(0, 1);
  // Extra spaces to prevent redundant % 
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: 1%%)
  lcd_1.setCursor(0, 1);
  lcd_1.print(relativeHumidity);
  lcd_1.print("%       ");
}

void lcdLoudness() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Loudness:");
  lcd_1.setCursor(0, 1);
  // Extra spaces to prevent redundant db 
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: 1dBB)
  lcd_1.setCursor(0, 1);
  lcd_1.print(decibel);
  lcd_1.print(" dB       ");
}