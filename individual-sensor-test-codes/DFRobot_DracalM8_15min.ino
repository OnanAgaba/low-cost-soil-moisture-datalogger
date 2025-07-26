/* Project: Monitoring and Logging Soil Conditions
Goal: This is an individual sensor code to test the functionality of the DFRobot and Dracal I2C sensors 
Author: Mulugeta Weldegebriel Hagos,
Co-Authors: Onan Agaba, Zahra Fatma-Kasmi,
Instructor: Elad Levintal, PhD,
TAs; Devi-Orozcco Fuentes,
     Thi Thuc Nguyen,
Last Modified: July 26, 2025
*/

// Load Libraries; Install them if you don't have them
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Define variables for the DFrobot sensor
const int AirValue = 570; // you need to calibrate this
const int WaterValue = 0;
int intervals = (AirValue - WaterValue) / 3;
int soilMoistureValue1 = 0;
int soilMoistureValue2 = 0;
String status1, status2;

unsigned long previousMillis = 0;          // Store the last time measurements were taken
const unsigned long interval = 900000UL;   // 15 minutes in milliseconds

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;

    // Read soil sensor 1
    soilMoistureValue1 = analogRead(A0);
    if (soilMoistureValue1 > WaterValue && soilMoistureValue1 < (WaterValue + intervals)) {
      status1 = "Very Wet";
    } else if (soilMoistureValue1 > (WaterValue + intervals) && soilMoistureValue1 < (AirValue - intervals)) {
      status1 = "Wet";
    } else if (soilMoistureValue1 < AirValue && soilMoistureValue1 > (AirValue - intervals)) {
      status1 = "Dry";
    }

    // Read soil sensor 2
    soilMoistureValue2 = analogRead(A1);
    if (soilMoistureValue2 > WaterValue && soilMoistureValue2 < (WaterValue + intervals)) {
      status2 = "Very Wet";
    } else if (soilMoistureValue2 > (WaterValue + intervals) && soilMoistureValue2 < (AirValue - intervals)) {
      status2 = "Wet";
    } else if (soilMoistureValue2 < AirValue && soilMoistureValue2 > (AirValue - intervals)) {
      status2 = "Dry";
    }

    // Print results
    Serial.print("sensor1: ");
    Serial.print(soilMoistureValue1);
    Serial.print(" ");
    Serial.print(status1);
    Serial.print(" :sensor2: ");
    Serial.print(soilMoistureValue2);
    Serial.print(" ");
    Serial.print(status2);

    // Read temperature and humidity
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    Serial.print("    Temp *C = "); Serial.print(t);
    Serial.print("\t\tHum. % = "); Serial.println(h);
  }
}
