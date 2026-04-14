/*
 * ============================================================================
 *  File Name:      sBus_to_serial_RP2040.ino
 *  Author:         RCEJ
 *  Date Created:   2026.04.02
 *  Last Modified:  2026.04.12
 *
 *  Description:
 *      Invert sBus signal so it can be processed as normal serial input.
 *
 *  Platform/MCU:
 *      Waveshare RP2040 Zero
 *
 *  Dependencies:
 *      None
 *
 *  Usage:
 *      Use on Raspberry Pico RP2040(Zero). 
 *      Signal sBus input on GPIO 5.
 *      Signal output via USB (serial port)
 * 
 *      Output can also be used in sBus_Serial_Monitor.html
 *
 *  Revision History:
 *      2026.04.12 First release
 *
 * ============================================================================
 */

#include <Arduino.h>
#define SBUS_RX_PIN 5

void setup() {
  Serial.begin(100000, SERIAL_8E2);

  Serial2.setRX(SBUS_RX_PIN);
  Serial2.setInvertRX(true);                 // No inverter needed
  Serial2.begin(100000, SERIAL_8E2);        

  delay(500);
}


void loop() {
  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}