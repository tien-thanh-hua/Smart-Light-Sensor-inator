#include <LiquidCrystal_I2C.h>  // LCD with I2C library. Don't use this if LCD does not use I2C connection
#include <DHT.h>                // DHT22 library

// the switch for the light bulb(s)
const int LIGHT_RELAY_PIN = 12;

// status LEDs for light switch sensors
const int LS_LED = 2;
const int US_LED = 3;
const int PIR_LED = 4;

// additional sensors for LCD output
const int BUTTON_PIN = 5;
const int TEMP_HUMID_PIN = A1;
const int LOUDNESS_PIN = A0;

// light switch sensors
const int PHOTORESISTOR_PIN = A2;
const int ECHO_PIN = 6;
const int TRIG_PIN = 7;
const int PIR_PIN = 8;

// 9x27: LCD address (some rare I2C modules may use 0x3F instead)
// 16 columns, 2 rows
LiquidCrystal_I2C lcd_1(0x27, 16, 2);

// DHT22 temperature + humidity sensor
const int DHT_TYPE = DHT22;
DHT dht(TEMP_HUMID_PIN, DHT_TYPE);

// global variables used to quickly display information in Serial Monitor or LCD
float celsius, humidity, decibel, cm;
int light;

boolean isPIRDetected;
boolean isLightOn = false;
int lightTimer = 0;

int buttonPushCounter = 1;  // counter for the number of button presses
int buttonState = 0;        // current state of the button
int lastButtonState = 0;    // previous state of the button

void setup() {
  // initialize pin to relay input, which controls the light state
  pinMode(LIGHT_RELAY_PIN, OUTPUT);

  // initialize pins of sensors
  pinMode(PHOTORESISTOR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // initialize button pin
  pinMode(BUTTON_PIN, INPUT);

  // initialize status LEDs' pins
  pinMode(LS_LED, OUTPUT);
  pinMode(US_LED, OUTPUT);
  pinMode(PIR_LED, OUTPUT);

  // initialize serial communication
  Serial.begin(9600);

  // initialize LCD display
  lcd_1.init();
  lcd_1.clear();
  lcd_1.backlight();  // comment this line to disable LCD backlight

  // initialize DHT22 sensor
  dht.begin();
}

void loop() {
  // PIR sensor returns a HIGH value when a moving object
  // with a higher temperature than the sensor's threshold
  // moves in its vicinity, and returns LOW after said object
  // stops moving or moves out of range, with a delay set to ~10s
  isPIRDetected = digitalRead(PIR_PIN);
  sensorLED(PIR_LED, isPIRDetected);

  // Ultrasonic sensor measure distance between itself
  // and an object in its range in cm. When the distance
  // is below a certain threshold, it will meet its condition
  cm = 0.01723 * readUltrasonicDistance(7, 6);
  boolean isUltrasonicDetected = (cm <= 150);
  sensorLED(US_LED, isUltrasonicDetected);

  // Uses a photoresistor to measure light intensity. Its analog value
  // drops as brightness increases, and vice versa. Once its reading
  // is above a certain threshold (meaning brightness reduces below a certain value)
  // it will meet the condition
  light = analogRead(PHOTORESISTOR_PIN);
  boolean isLightDetected = (light >= 512);
  sensorLED(LS_LED, isLightDetected);

  // additional sensor readings, then outputted to LCD and/or Serial Monitor
  humidity = dht.readHumidity();
  celsius = dht.readTemperature();
  decibel = getDecibel();

  if (isPIRDetected && isUltrasonicDetected && isLightDetected) {  // all 3 conditions are met
    digitalWrite(LIGHT_RELAY_PIN, HIGH);
    isLightOn = true;
    lightTimer = 0;                                // resets timer
  } else if (isUltrasonicDetected && isLightOn) {  // Ultrasonic sensor and light is still on
    digitalWrite(LIGHT_RELAY_PIN, HIGH);
    lightTimer = 0;          // resets timer
  } else {                   // Ultrasonic off, starts timer
    if (lightTimer < 100) {  // 100 loops = 10s of delay + additional code time should equal ~15s
      lightTimer++;
    } else {  // upon timer reaching 100, turns off light
      digitalWrite(LIGHT_RELAY_PIN, LOW);
      isLightOn = false;
    }
  }

  // displays temp + humidity + loudness readings on LCD
  lcdButton();

  // outputs sensor values to Serial Monitor
  serialReading();

  delay(38);  // Wait for 12ms (ultrasonic) + 50ms (button) + 38ms = 100ms per loop
}

float getDecibel() {
  unsigned long startMillis = millis();  // Start of sample window
  float peakToPeak = 0;                  // peak-to-peak level

  unsigned int signalMax = 0;     // minimum value
  unsigned int signalMin = 1023;  // maximum value

  // collect data for 50 mS
  while (millis() - startMillis < 50) {     // 50: sample window time
    int sample = analogRead(LOUDNESS_PIN);  // get reading from microphone
    if (sample < 1023)                      // toss out spurious readings
    {
      if (sample > signalMax) {
        signalMax = sample;  // save just the max levels
      } else if (sample < signalMin) {
        signalMin = sample;  // save just the min levels
      }
    }
    // additional debug info
    //Serial.println(sample);
  }

  peakToPeak = signalMax - signalMin;                          // max - min = peak-peak amplitude
  return map(peakToPeak, 0, 900, 49, 90); 
  //return analogRead(LOUDNESS_PIN);
}

float readUltrasonicDistance(int sensorLEDPin, int echoPin) {  // 12ms delay per loop
  pinMode(sensorLEDPin, OUTPUT);                               // Clear the trigger
  digitalWrite(sensorLEDPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(sensorLEDPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorLEDPin, LOW);
  pinMode(echoPin, INPUT);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH);
}

void serialReading() {
  Serial.print("PIR: ");
  Serial.print(isPIRDetected);
  Serial.print(" | Photoresistor: ");
  Serial.print(light);
  Serial.print(" | Distance: ");
  Serial.print(cm);
  Serial.println("cm");
  Serial.print(celsius);
  Serial.print("C | ");
  Serial.print(humidity);
  Serial.print("% | ");
  Serial.print(decibel);
  Serial.println("dB");
  Serial.println(" ");
}

void sensorLED(int sensorLEDPin, boolean value) {
  if (value == true) {
    digitalWrite(sensorLEDPin, HIGH);
  } else {
    digitalWrite(sensorLEDPin, LOW);
  }
}

void lcdButton() {
  // read the pushbutton input pin:
  buttonState = digitalRead(BUTTON_PIN);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      if (buttonPushCounter == 3) {
        // resets counter, starting new cycle
        buttonPushCounter = 1;
      } else {
        buttonPushCounter++;
      }
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
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;

  // each cycle consists of 3 display states:
  // 1. Temperature in degree Celsius
  // 2. Relative humidity in percentage
  // 3. Loudness in decibel
  switch (buttonPushCounter) {
    case 1:
      lcdTemp();
      break;
    case 2:
      lcdHumidity();
      break;
    case 3:
      lcdLoudness();
      break;
  }
}

void clearLCDFirstLine() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("                ");  // "clears" the first line without using lcd.clear()
}

void lcdTemp() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Temperature:");
  lcd_1.setCursor(0, 1);
  lcd_1.print(celsius);
  lcd_1.print((char)223);  // degree symbol for hd44780 display
  // Extra spaces prevent redundant " C"
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: -110.46 C -> -75.02 CC -> 1.24 CCCC)
  lcd_1.print("C       ");
}

void lcdHumidity() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Humidity:");
  lcd_1.setCursor(0, 1);
  lcd_1.print(humidity);
  // Extra spaces to prevent redundant %
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: 1%%)
  lcd_1.print("%       ");
}

void lcdLoudness() {
  lcd_1.setCursor(0, 0);
  lcd_1.print("Loudness:");
  lcd_1.setCursor(0, 1);
  lcd_1.print(decibel);
  // Extra spaces to prevent redundant db
  // in case new value has more than 1 digit
  // difference compared to previous value
  // (example: 1dBB)
  lcd_1.print(" dB       ");
}