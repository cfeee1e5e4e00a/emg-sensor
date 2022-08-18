/*
 * Find and connect to UNIor BLE module, transmit data values to serial port 
 * 
 * OOO "COMSIB", 2022
 * http://boslab.ru
 * 
 * The repository for the ESP32 BLE support for Arduino: https://github.com/nkolban/esp32-snippets 
 */
 
#define _PRINT_INFO_

// #include <BLEDevice.h>
// #include <BLEScan.h>
#include "emg.h"

void setup() 
{  
  Serial.begin(74880);
  EMG::init("CF:41:70:83:A3:9F");
} // End of setup.


// This is the Arduino main loop function.
void loop() 
{
  Serial.println(EMG::readValue());
  // delay(100);
} // End of loop
