/*
 * ============================================================================
 *  File Name:      sBus_debug_RP2040.ino
 *  Author:         RCEJ
 *  Date Created:   2026.04.02
 *  Last Modified:  2026.04.14
 *
 *
 *  Description:
 *      sBus signal is writen to USB serial COM port in human readable debug format.
 *      Use a terminal application to display the output.
 *
 *  Platform/MCU:
 *      Waveshare RP2040 Zero
 *
 *  Dependencies:
 *      Bolderflight sbus library:  https://github.com/bolderflight/sbus
 *
 *  Usage:
 *      Use on Raspberry Pico RP2040(Zero). 
 *      Signal input on GPIO 5.
 *      Debug 16 channel sBus signal with output via USB serial port.
 * 
 *
 *  Revision History:
 *      2026.04.14 First release
 * ============================================================================
 */

#include <Arduino.h>
#include "sbus.h"

#define SBUS_RX_PIN 5

bfs::SbusRx sbus_rx(&Serial2);
bfs::SbusData sbusData;

void setup() {
  Serial.begin(115200);

  Serial2.setRX(SBUS_RX_PIN);
  Serial2.setInvertRX(true);                 // No external hardware inverter needed
  Serial2.begin(100000, SERIAL_8E2);        

  sbus_rx.Begin();
  
  delay(1000);
  Serial.println("sBus debug on RP2040.");
}

void setup1() {
  delay(1000);
}

void loop() {
  if (sbus_rx.Read()) {
    sbusData = sbus_rx.data();
  }
}

void loop1() {
  static unsigned long lastupdatetime = millis();

  if ((millis() - lastupdatetime) > 500) {
      for (int i = 0; i < 16; i++) {
        Serial.print("Ch");
        Serial.print(i+1);
        Serial.print(":");
        Serial.print(sbusData.ch[i]);
        Serial.print(i < 15 ? ", " : "");
      }
      Serial.println();
      lastupdatetime = millis();
  }
}

