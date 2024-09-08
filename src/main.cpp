#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// DS18B20 Pins
#define ONE_WIRE_BUS_A2 A1
#define ONE_WIRE_BUS_A3 A2

// TFT Display Pins
#define TFT_CS     10
#define TFT_RST    9  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC     8

// Analog Input Pin
#define VIN        A4

// Button Pin
#define BUTTON_PIN 4

// Constants
const float VCC = 5.0;
const float QOV = 2.5; // Quiescent Output Voltage (around 2.5V for no current)
const float SENSITIVITY = 0.04; // 40mV per Ampere for ACS758 50B
const float DEADZONE_THRESHOLD = 0.02; // To filter out noise


// Variables
float voltage;
float voltage_raw;
float current;
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
bool running = false;
int buttonState;
int lastButtonState = LOW;
int buttonPressCount = 0;

float cutOff;
// Previous Values to Reduce Flicker
float prevTempA2 = -1000;
float prevTempA3 = -1000;
float prevVoltage = -1000;
float prevCurrent = -1000;
unsigned long prevElapsedTime = 0;

// Create display object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Setup a oneWire instance to communicate with the sensors
OneWire oneWireA2(ONE_WIRE_BUS_A2);
OneWire oneWireA3(ONE_WIRE_BUS_A3);

// Pass the oneWire reference to Dallas Temperature
DallasTemperature sensorsA2(&oneWireA2);
DallasTemperature sensorsA3(&oneWireA3);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Initialize DS18B20 sensors
  sensorsA2.begin();
  sensorsA3.begin();

  // Initialize TFT Display
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_WHITE); // Set the screen background to white

  // Initial static text setup
  tft.setTextColor(ST7735_BLACK); // Set text color to black
  tft.setTextSize(2); // Increase font size

  tft.setCursor(10, 10);
  tft.print("Bat:");

  tft.setCursor(10, 40);
  tft.print("Mo:");

  tft.setCursor(10, 70);
  tft.print("V:");

  tft.setCursor(10, 100);
  tft.print("A:");

  tft.setCursor(10, 130);
  tft.print("T:");

  // Setup button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void checkButtonState() {
  buttonState = digitalRead(BUTTON_PIN);

  // Check if button was pressed
  if (buttonState == HIGH && lastButtonState == LOW) {
    buttonPressCount++;
    if (buttonPressCount == 1) {
      // Start the timer
      running = true;
      startTime = millis(); // Set start time at the moment button is pressed
    } else if (buttonPressCount == 2) {
      // Stop the timer
      running = false;
      elapsedTime = millis() - startTime;
    } else if (buttonPressCount == 3) {
      // Reset the timer
      buttonPressCount = 0;
      elapsedTime = 0;
      startTime = 0;
      running = false;
    }
  }
  lastButtonState = buttonState;
}

void displayTime(unsigned long elapsed) {
  // Calculate minutes, seconds, and milliseconds
  unsigned int minutes = (elapsed / 60000) % 60;
  unsigned int seconds = (elapsed / 1000) % 60;
  unsigned int milliseconds = (elapsed % 1000) / 10; // Divide by 10 to get two digits

  // Print time in mm:ss:ms format
  tft.setCursor(44, 130);
  tft.setTextColor(ST7735_WHITE); // Overwrite previous value with white
  tft.print(prevElapsedTime / 60000); // Display previous minutes
  tft.print(":");
  if ((prevElapsedTime / 1000) % 60 < 10) tft.print("0"); // Add leading zero if needed
  tft.print((prevElapsedTime / 1000) % 60); // Display previous seconds
  tft.print(":");
  if ((prevElapsedTime % 1000) / 10 < 10) tft.print("0"); // Add leading zero if needed
  tft.print((prevElapsedTime % 1000) / 10); // Display previous two-digit milliseconds

  tft.setCursor(44, 130);
  tft.setTextColor(ST7735_BLACK); // Set text color back to black
  tft.print(minutes);
  tft.print(":");
  if (seconds < 10) tft.print("0");
  tft.print(seconds);
  tft.print(":");
  if (milliseconds < 10) tft.print("0");
  tft.print(milliseconds);
}

void loop() {
  // Check button state at the beginning of the loop
  checkButtonState();

  // If the timer is running, calculate elapsed time
  if (running) {
    elapsedTime = millis() - startTime;
  }

  // Request temperature measurements
  sensorsA2.requestTemperatures();
  sensorsA3.requestTemperatures();

  // Check button state after temperature request
  checkButtonState();

  // Read temperatures
  float tempA2 = sensorsA2.getTempCByIndex(0);
  float tempA3 = sensorsA3.getTempCByIndex(0);

  // Check button state after reading temperatures
  checkButtonState();

  // Read voltage and calculate current
  voltage_raw = (5.0 / 1023.0) * analogRead(VIN);

  float voltage_diff = voltage_raw - QOV; 
  // Apply a dead zone around QOV to avoid negative currents close to zero
  if (abs(voltage_diff) < DEADZONE_THRESHOLD) {
    current = 0.0;
  } else {
    // If outside dead zone, calculate the actual current
    current = voltage_diff / SENSITIVITY;
  }


  // Check button state after reading voltage
  checkButtonState();

  // Update Bat (Temp A2) on TFT if changed
  if (tempA2 != prevTempA2) {
    tft.setCursor(55, 10); // Adjusted cursor position
    tft.setTextColor(ST7735_WHITE); // Overwrite previous value with white
    tft.print(prevTempA2, 2);
    tft.setCursor(55, 10); // Adjusted cursor position
    tft.setTextColor(ST7735_BLACK); // Set text color back to black
    tft.print(tempA2, 2);
    prevTempA2 = tempA2;  // Update previous value
  }

  // Check button state after updating tempA2
  checkButtonState();

  // Update Mo (Temp A3) on TFT if changed
  if (tempA3 != prevTempA3) {
    tft.setCursor(50, 40); // Adjusted cursor position
    tft.setTextColor(ST7735_WHITE); // Overwrite previous value with white
    tft.print(prevTempA3, 2);
    tft.setCursor(50, 40); // Adjusted cursor position
    tft.setTextColor(ST7735_BLACK); // Set text color back to black
    tft.print(tempA3, 2);
    prevTempA3 = tempA3;  // Update previous value
  }

  // Check button state after updating tempA3
  checkButtonState();

  // Update Voltage on TFT if changed
  if (voltage_diff != prevVoltage) {
    tft.setCursor(40, 70); // Adjusted cursor position
    tft.setTextColor(ST7735_WHITE); // Overwrite previous value with white
    tft.print(prevVoltage, 3);
    tft.setCursor(40, 70); // Adjusted cursor position
    tft.setTextColor(ST7735_BLACK); // Set text color back to black
    tft.print(voltage_raw, 3);
    prevVoltage = voltage_raw;  // Update previous value
  }
  // Check button state after updating voltage
  checkButtonState();

  // Update Current on TFT if changed
 if (current != prevCurrent) {
    tft.setCursor(40, 100); // Adjusted cursor position
    tft.setTextColor(ST7735_WHITE); // Overwrite previous value with white
    tft.print(prevCurrent, 2);
    tft.setCursor(40, 100); // Adjusted cursor position
    tft.setTextColor(ST7735_BLACK); // Set text color back to black
    tft.print(current, 2);
    prevCurrent = current;  // Update previous value
  }

  // Check button state after updating current
  checkButtonState();

  // Update Timer on TFT if changed
  if (elapsedTime != prevElapsedTime) {
    displayTime(elapsedTime);
    prevElapsedTime = elapsedTime;  // Update previous value
  }

  // Display temperature and current on Serial Monitor
  Serial.print("Bat: ");
  Serial.print(tempA2);
  Serial.println(" °C");

  Serial.print("Mo: ");
  Serial.print(tempA3);
  Serial.println(" °C");

  Serial.print("Voltage Raw: ");
  Serial.println(voltage_raw, 3); // Print voltage with 3 decimal places

   if (current != 0) {
    Serial.print("Current: ");
    Serial.print(current, 2); // Print the current with 2 decimal places
    Serial.println(" A");
  } else {
    Serial.println("No significant current");
  }

  delay(100);
}
