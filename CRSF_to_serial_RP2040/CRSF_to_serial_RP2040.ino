/*
 * ============================================================================
 *  File Name:      CRSF_to_serial_RP2040.ino
 *  Author:         RCEJ
 *  Date Created:   2026.04.02
 *  Last Modified:  2026.04.14
 *
 *  Creative Common License:
 *      Attribution-NonCommercial-ShareAlike
 *      https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 *
 *  Description:
 *      Pipe CSRF serial signal to USB serial COM port.
 *
 *  Platform/MCU:
 *      Waveshare RP2040 Zero
 *
 *  Dependencies:
 *      None
 *
 *  Usage:
 *      Use on Raspberry Pico RP2040(Zero). 
 *      Signal input on GPIO 5.
 *      Signal output via USB (serial port)
 * 
 *      Output can also be used in CRSF_Serial_Monitor.html
 *
 *  Revision History:
 *      2026.04.14 First release
 * ============================================================================
 */

#include <Arduino.h>
#define RX_PIN 5


void setup() {
  Serial.begin(420000, SERIAL_8N1);
  
  Serial2.setRX(RX_PIN);
  Serial2.begin(420000, SERIAL_8N1);        
  
  delay(500);
}


void loop() {
  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}