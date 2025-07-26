/* Project: Monitoring and Logging Soil Conditions
Goal: This is an individual sensor code to test the functionality of the TR-HTSE sensor
Author: Zahra Fatma-Kasmi,
Co-Authors: Onan Agaba, Mulugeta Weldegebriel Hagos,
Instructor: Elad Levintal, PhD,
TAs; Devi-Orozcco Fuentes,
     Thi Thuc Nguyen,
Last Modified: July 26, 2025
*/

// Load Libraries; Install them if you don't have them
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // Create ADS1115 instance

// Constants
const unsigned long MEASUREMENT_INTERVAL = 10000UL;  // 15 minutes = 900,000 ms
unsigned long lastMeasurementTime = 0;

// Data Collection Timing
unsigned long previousMillis = 0;          // store the last time measurements were taken
const unsigned long interval = 10000UL;

void setup() {
  Serial.begin(9600);

  // Wait for Serial Monitor to open (only for boards like Leonardo, Micro, etc.)
  while (!Serial) {
    delay(10);  // Wait for serial connection
  }

  if (!ads.begin()) {
    Serial.println("ADS1115 not detected. Check wiring!");
    while (1);  
  }

  ads.setGain(GAIN_TWO);  // ±2.048V range

  // Force first measurement immediately
  lastMeasurementTime = millis() - MEASUREMENT_INTERVAL;
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastMeasurementTime >= MEASUREMENT_INTERVAL) {
    lastMeasurementTime = currentMillis;

    // Read raw ADC values
    int16_t adc0 = ads.readADC_SingleEnded(0);  // A0 - Temperature
    int16_t adc1 = ads.readADC_SingleEnded(1);  // A1 - Moisture

    // Convert to voltage
    float voltage0 = ads.computeVolts(adc0);  // Temperature
    float voltage1 = ads.computeVolts(adc1);  // Moisture

    // Apply conversion formulas from TR-HTSE manual
    float temperature = 60.0 * voltage0 - 40.0;
    float moisture = 50.0 * voltage1;

    // Display results
    Serial.print("Soil Moisture: ");
    Serial.print(moisture, 1);
    Serial.print(" %\t");

    Serial.print("Soil Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");
  }


}
