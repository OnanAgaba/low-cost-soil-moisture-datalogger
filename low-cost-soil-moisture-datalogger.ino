/* Project: Monitoring and Logging Soil Conditions
Goal: Evaluating the performance of 4 soil moisture sensor types in Arid Conditions
Author: Onan Agaba,
Co-Authors: Mulugeta Weldegebriel Hagos, Zahra Fatma-Kasmi,
Instructor: Elad Levintal, PhD,
TAs; Devi-Orozcco Fuentes,
     Thi Thuc Nguyen,
Last Modified: July 10, 2025
*/


// Load Libraries
#include <SDI12.h>                // Loads the SDI-12 protocol library, used for communicating with the TDR-305N soil sensor
#include <Wire.h>                 // Loads the I2C communication library, used by many I2C devices (e.g., ADS1115, SHT31)
#include <Adafruit_ADS1X15.h>     // Loads the library for ADS1115/ADS1015 ADC modules, used to read analog voltages from the TRHTSE sensor
#include <Arduino.h>              // Core Arduino functions (can be optional,but we included for proper interaction between different function)
#include "Adafruit_SHT31.h"       // Loads the library for the SHT31 temperature and humidity sensor via I2C, used for Dracal I2C sensor
#include <Notecard.h>             // Library for the Notecard; for logging communication between the system and the hub
#include <SPI.h>                  // Loads the SPI (Serial Peripheral Interface) communication protocol
#include <SD.h>                   // Loads the SD card library to read/write files on an SD card
#include <RTClib.h>               // Loads the real-time clock library



// Define Configuration Constants
#define DATA_PIN 6                  // Pin connected to SDI-12 data bus/line (TDR-305N)
#define POWER_PIN -1                // Not used, if you want to control sensor power, assign a valid pin
File dataFile;



// Initialize Sensor Objects 
SDI12 mySDI12(DATA_PIN);                   // 1. TDR-305N: SDI-12 Sensor
Adafruit_ADS1115 ads;                      // 2. TRHTSE: Analog Sensor via ADS1115 with default I2C address 0x48  
Adafruit_SHT31 sht31 = Adafruit_SHT31();   // 3. Dracal SHT31: I2C Sensor



// Sensor Availability Flags
bool ads_available = false;
bool sht31_available = false;
bool sdi12_available = false;



// Analog Sensors: DFROBOT Capacitance Soil Moisture Sensors
const int AirValue = 570;        // ADC reading in dry air (calibrate for your sensor)
const int WaterValue = 0;        // ADC reading in water (calibrate)
int intervals = (AirValue - WaterValue) / 3;
int soilMoistureValue1 = 0;
int soilMoistureValue2 = 0;
String status1, status2;



// Data Collection Timing - Both in milliseconds and real-time
unsigned long previousMillis = 0;           // store the last time measurements were taken
const unsigned long interval = 900000UL;     // 10 seconds in milliseconds for testing (change to 900000UL for 15 minutes at the Final Deployment Stage)
char buffer [24];  
RTC_DS3231 rtc;


// Taking Measurements from SDI-12
byte addressRegister[8] = {
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000
};
uint8_t numSensors = 0;



// Converts Allowable Address Characters '0'-'9', 'a'-'z', 'A'-'Z',
// To a Decimal Number Between 0 and 61 (inclusive) to Cover the 62 possible addresses
byte charToDec(char i){
  if((i >= '0') && (i <= '9')) return i - '0';
  if((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if((i >= 'A') && (i <= 'Z')) return i - 'A' + 37;
  else return i;
}



// Maps a Decimal Number Between  0 and 61 (inclusive) to
// Allowable Address Characters '0'-'9', 'a'-'z', 'A'-'Z',
char decToChar(byte i){
  if((i >= 0) && (i <= 9)) return i + '0';
  if((i >= 10) && (i <= 36)) return i + 'a' - 10;
  if((i >= 37) && (i <= 62)) return i + 'A' - 37;
  else return i;
}


// Consume the Address
void printBufferToScreen(){
  String buffer = "";
  mySDI12.read(); // consume address
  while(mySDI12.available()){
    char c = mySDI12.read();
    if(c == '+'){
      buffer += ',';
    }
    else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
  }
  Serial.print(buffer);
}



// Gets Identification Information from a Sensor and Prints it to the Serial Port
// Expects a Character between '0'-'9', 'a'-'z', or 'A'-'Z'.
String getInfo(char i){
  String command = "";
  command += (char) i;
  command += "I!";
  mySDI12.sendCommand(command);
  delay(30);

  String buffer = "";
  mySDI12.read(); // consume address
  while(mySDI12.available()){
    char c = mySDI12.read();
    if(c == '+'){
      buffer += ',';
    }
    else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
  }
  return buffer;
}



// Take Measurements
String takeMeasurement(char i){
  String command = "";
  command += i;
  command += "M!"; // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command);
  delay(30);

  // Wait for acknowlegement with format [address][ttt (3 char, seconds)][number of measurements available, 0-9]
  String sdiResponse = "";
  delay(30);
  while (mySDI12.available())  // build response string
  {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r'))
    {
      sdiResponse += c;
      delay(5);
    }
  }
  mySDI12.clearBuffer();

  // Find out how long we have to wait (in seconds).
  uint8_t wait = 0;
  wait = sdiResponse.substring(1,4).toInt();

  // Set up the number of results to expect
  // int numMeasurements =  sdiResponse.substring(4,5).toInt();
  unsigned long timerStart = millis();
  while((millis() - timerStart) < (1000 * wait)){
    if(mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      mySDI12.clearBuffer();
      break;
    }
  }
  // Wait for anything else and clear it out
  delay(30);
  mySDI12.clearBuffer();

  // In this example we will only take the 'DO' measurement
  command = "";
  command += i;
  command += "D0!"; // SDI-12 command to get data [address][D][dataOption][!]
  mySDI12.sendCommand(command);
  while(!mySDI12.available()>1); // wait for acknowlegement
  delay(300); // let the data transfer
  
  String buffer = "";
  mySDI12.read(); // consume address
  while(mySDI12.available()){
    char c = mySDI12.read();
    if(c == '+'){
      buffer += ',';
    }
    else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
  }
  mySDI12.clearBuffer();
  return buffer;
}




// Check the Activity at a Particular Address
// Expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i){

  String myCommand = "";
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(30);
    if(mySDI12.available()) {            // If we here anything, assume we have an active sensor
      printBufferToScreen();
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}

// this quickly checks if the address has already been taken by an active sensor
boolean isTaken(byte i){
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  return addressRegister[j] & (1<<k); // return bit status
}

// this sets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setTaken(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] |= (1 << k);
  return !initStatus; // return false if already taken
}

// This unsets the bit in the proper location within the addressRegister
// To record that the sensor is active and the address is taken.
boolean setVacant(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] &= ~(1 << k);
  return initStatus; // return false if already vacant
}



// Function to Log Data to SD card
void logDataToSD(String dataString) {
  dataFile = SD.open("Soillog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    Serial.println("Data logged to SD card successfully.");
  } else {
    Serial.println("Error opening Soillog.csv for writing.");
  }
}



// Whole Setup for our Soil Moisture Logger
void setup() {
  Serial.begin(115200);
  
  // Wait for Serial Monitor to open (for boards like Leonardo, Micro, etc.)
  while (!Serial) {
    delay(10);
  }

  Wire.begin();                           // Start I2C communication
  mySDI12.begin();                        // Start SDI-12 communication for TDR-305N
  pinMode(LED_BUILTIN, OUTPUT);           // This is our heartbeat
  
  
  
  // Initialize Real-time clock
 if (!rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
  }
 if (rtc.lostPower()) {
   Serial.println("RTC lost power, set the time!");

  // Set RTC to the date and time the sketch was compiled
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  }



  // Initialize SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("SD card initialization failed!");
    // Continue without SD card
  } else {
    Serial.println("SD card initialization done.");
  }

  // 1. Initialize TRHTSE Sensor (ADS1115)
  ads_available = false;
  if (!ads.begin()) {
    Serial.println("ADS1115 not detected. Check wiring! System will continue without it.");
    ads_available = false;
  } else {
    Serial.println("ADS1115 initialized successfully.");
    ads.setGain(GAIN_TWO);  // ±2.048V range is good for most sensors
    ads_available = true;
  }

  // Force first measurement immediately
  previousMillis = millis() - interval; 

  // 2. Initialize I2C Dracal sensors
  Serial.println("SHT31 test");
  sht31_available = false;
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31. System will continue without it.");
    sht31_available = false;
  } else {
    Serial.println("SHT31 initialized successfully.");
    sht31_available = true;
  }

  // 3. Initialize TDR 305-N
  //Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500); // allow things to settle

  // Power the sensors;
  if(POWER_PIN > 0){
    //Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
  }
  
  Serial.println("Scanning all addresses, please wait...");
  for(byte i = '0'; i <= '9'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space 0-9
  for(byte i = 'a'; i <= 'z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space a-z
  for(byte i = 'A'; i <= 'Z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space A-Z

  boolean found = false;
  for(byte i = 0; i < 62; i++){
    if(isTaken(i)){
      found = true;
      Serial.print("First address found:  ");
      Serial.println(decToChar(i));
      Serial.print("Total number of sensors found:  ");
      Serial.println(numSensors);
      break;
    }
  }
  if(!found) {
    Serial.println("No SDI-12 sensors found, please check connections. System will continue without SDI-12 sensors.");
    sdi12_available = false;
  } else {
    sdi12_available = true;
  }

  Serial.println();
  Serial.println(
    "Datetime,"                           // Real Time from RTC
    "Time(s),"                            // Timestamp
    "SDI12_Status,SDI12_ID,"              // TDR-305N Sensor status and ID
    "VWC(%),SoilTemp(°C),Permittivity,"   // TDR measurements
    "BulkEC(dS/m),PoreEC(dS/m),"          // More TDR measurements
    "TRHTSE_Status,TRHTSE_Temp(°C),"      // TRHTSE via ADS1115
    "TRHTSE_VWC(%),"           
    "SHT31_Status,SHT31_Temp(°C),"        // Dracal SHT31
    "SHT31_RH(%),"
    "DFRobot1_Status,DFRobot1_Raw,DFRobot1_Status_Text,"
    "DFRobot2_Status,DFRobot2_Raw,DFRobot2_Status_Text"
  );
  Serial.println("------------------------------------------------------------------------------------------------------------------------------------------");

  // Always write the header to SD card
  dataFile = SD.open("Soillog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(
        "Datetime,"
        "Time(s),"
        "SDI12_Status,SDI12_ID," 
        "VWC(%),SoilTemp(°C),Permittivity,BulkEC(dS/m),PoreEC(dS/m),"
        "TRHTSE_Status,TRHTSE_Temp(°C),TRHTSE_VWC(%),"
        "SHT31_Status,SHT31_Temp(°C),SHT31_RH(%),"
        "DFRobot1_Status,DFRobot1_Raw,DFRobot1_Status_Text,DFRobot2_Status,DFRobot2_Raw,DFRobot2_Status_Text"
      );
    dataFile.close();
    Serial.println("CSV header written to SD card.");
  } else {
    Serial.println("Error creating Soillog.csv.");
  }
}



// The Loop Function Runs Over and Over Again Forever
void loop() {
  // Check if it's time to take measurements (every 15 minutes)
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;

    // Step 1: LED HIGH
    digitalWrite(LED_BUILTIN, HIGH);

    // Step 2: Time
    DateTime now = rtc.now();
    sprintf(buffer, "%04u-%02u-%02u %02u:%02u:%02u", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());
    String datetimeStr = String(buffer);
    String dataString = datetimeStr + "," + String(currentMillis / 1000) + ","; // Combining real-time together with milliseconds

    // Step 3: Reading Sensor Data
    // Sensor 1: SDI-12 (TDR-305N)
    String sdi12DataBlock = "";
    bool foundSDI12 = false;

    if (sdi12_available) {
      for (char i = '0'; i <= '9'; i++) {
        if (isTaken(i)) {
          sdi12DataBlock += getInfo(i) + ",";
          sdi12DataBlock += takeMeasurement(i) + ",";
          foundSDI12 = true;
        }
      }
      for (char i = 'a'; i <= 'z'; i++) {
        if (isTaken(i)) {
          sdi12DataBlock += getInfo(i) + ",";
          sdi12DataBlock += takeMeasurement(i) + ",";
          foundSDI12 = true;
        }
      }
      for (char i = 'A'; i <= 'Z'; i++) {
        if (isTaken(i)) {
          sdi12DataBlock += getInfo(i) + ",";
          sdi12DataBlock += takeMeasurement(i) + ",";
          foundSDI12 = true;
        }
      }

      if (!foundSDI12) {
        sdi12DataBlock = "SDI12_ERROR,N/A,N/A,N/A,N/A,N/A,N/A,";
      }
    } else {
      sdi12DataBlock = "SDI12_NOT_FOUND,N/A,N/A,N/A,N/A,N/A,N/A,";
    }

    dataString += sdi12DataBlock;

    // Sensor 2: TRHTSE (ADS1115)
    String trhStatus = "TRHTSE_Found";
    float trhTemp = 0.0, trhVWC = 0.0, trhSoilEC = 0.0;

    if (ads_available) {
      // Read raw ADC values
      int16_t adc0 = ads.readADC_SingleEnded(0); // A0 - Temperature
      int16_t adc1 = ads.readADC_SingleEnded(1); // A1 - Moisture
      
      // Convert to voltage
      float voltage0 = ads.computeVolts(adc0); // Temperature
      float voltage1 = ads.computeVolts(adc1); // Moisture

      // Apply conversion formulae from the TR-HTSE Manual        
      if (voltage0 >= 0 && voltage0 <= 5.0){
          trhTemp = 60.0 * voltage0 - 40.0;
      } else { 
        trhStatus = "ERROR"; 
        trhTemp = 0.0; 
      }

      if (voltage1 >= 0 && voltage1 <= 5.0){
        trhVWC = 50.0 * voltage1;
      } else { 
        trhStatus = "ERROR"; 
        trhVWC = 0.0; 
      }

    } else {
      trhStatus = "TRH_NOT_FOUND";
    }
    // Format and append data string
    dataString += trhStatus + "," + String(trhTemp, 2) + "," + String(trhVWC, 2) + "," ;

    // Sensor 3: SHT31 I2C Dracal
    String sht31Status = "SHT31_FOUND";
    float sht31Temp = 0.0, sht31RH = 0.0;

    if (sht31_available) {
      sht31Temp = sht31.readTemperature();
      sht31RH = sht31.readHumidity();
      
      // Add NaN checks and status updates
      if (isnan(sht31Temp) || isnan(sht31RH)) {
        sht31Status = "ERROR";
        sht31Temp = sht31RH = 0.0;
        sht31RH = 0.0;
      }
    } else {
      sht31Status = "NOT_FOUND";
      sht31Temp = sht31RH = 0.0;
    }
    // Append to dataString
    dataString += sht31Status + "," + String(sht31Temp, 2) + "," + String(sht31RH, 2) + ",";

    // Sensor 4: DFROBOT Sensors - Using the same logic as individual code
    // Read soil sensor 1
    soilMoistureValue1 = analogRead(A0);
    soilMoistureValue2 = analogRead(A1);

    // Calculate moisture statuses
    if (soilMoistureValue1 > WaterValue && soilMoistureValue1 < (WaterValue + intervals)) {
      status1 = "Very Wet";
    } else if (soilMoistureValue1 > (WaterValue + intervals) && soilMoistureValue1 < (AirValue - intervals)) {
      status1 = "Wet";
    } else if (soilMoistureValue1 < AirValue && soilMoistureValue1 > (AirValue - intervals)) {
      status1 = "Dry";
    } else {
      status1 = "Unknown";
    }

    // Read soil sensor 2
    soilMoistureValue2 = analogRead(A1);
    if (soilMoistureValue2 > WaterValue && soilMoistureValue2 < (WaterValue + intervals)) {
      status2 = "Very Wet";
    } else if (soilMoistureValue2 > (WaterValue + intervals) && soilMoistureValue2 < (AirValue - intervals)) {
      status2 = "Wet";
    } else if (soilMoistureValue2 < AirValue && soilMoistureValue2 > (AirValue - intervals)) {
      status2 = "Dry";
    } else {
      status2 = "Unknown";
    }

    // Append to dataString:
    dataString += "DFROBOT1_FOUND,";                   // DFRobot1_Status
    dataString += String(soilMoistureValue1) + ",";
    dataString += status1 + ",";
    dataString += "DFROBOT2_FOUND,";                   // DFRobot2_Status
    dataString += String(soilMoistureValue2) + ",";
    dataString += status2;


    //Step 4: Output
    Serial.println(dataString);
    logDataToSD(dataString);

    // Step 5: LED OFF
    digitalWrite(LED_BUILTIN, LOW);
  }
  
  // Small delay to prevent overwhelming the system
  delay(1000);
}
