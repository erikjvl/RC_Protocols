/*
 * ============================================================================
 *  File Name:      CRSF_joystick_RP2040.ino
 *  Author:         RCEJ
 *  Date Created:   2026.04.02
 *  Last Modified:  2026.04.12
 *
 *  Creative Common License:
 *      Attribution-NonCommercial-ShareAlike
 *      https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 *
 *  Description:
 *      Convert 12 channels CRSF serial signal to joystick input.
 *      Channel 1~8 analog output
 *      Channel 9~10 multiplexed 2x3 two position switches. Multiplexed 68%, 27% and 3% per channel. 
 *      Channel 11~12 multiplexed 2x3 three position switches. Multiplexed 68%, 27% and 3% per channel.
 *      Three position switches are outputted in a tri-state compatible order (low-high-mid).
 *      Multi threaded over the MCU 2 cores. High speed update of first four channels.
 *
 *  Platform/MCU:
 *      Waveshare RP2040 Zero
 *
 *  Dependencies:
 *      Adafruit_TinyUSB (set in Arduino IDE menu: Tools, USB Stack)
 *
 *  Usage:
 *      Use on a Raspberry Pico RP2040(Zero). 
 *      Signal CRSF input on GPIO 5.
 *      Joystick output via USB.
 *      Support for 12 multiplexed switches.
 *
 *  Revision History:
 *      2026.04.12 First release
 *
 * ============================================================================
 */


#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

// -------------------------
// CRSF settings
// -------------------------
#define CRSF_RX_PIN 5
#define CRSF_BAUD 420000
#define CRSF_ADDR_RX 0xC8
#define CRSF_TYPE_CHANNELS 0x16

// CRSF values 172..1811 (11-bit)
uint16_t crsfChannels[16] = { 0 };

// -------------------------
// CRSF parser
// -------------------------
enum {
  CRSF_STATE_SYNC,
  CRSF_STATE_LEN,
  CRSF_STATE_TYPE,
  CRSF_STATE_PAYLOAD,
  CRSF_STATE_CRC
};

uint8_t crsfState = CRSF_STATE_SYNC;
uint8_t crsfLen = 0;
uint8_t crsfType = 0;
uint8_t crsfPayload[32];
uint8_t crsfIndex = 0;

void processCRSFByte(uint8_t b) {
  switch (crsfState) {
    case CRSF_STATE_SYNC:
      if (b == CRSF_ADDR_RX) {
        crsfState = CRSF_STATE_LEN;
      }
      break;

    case CRSF_STATE_LEN:
      crsfLen = b;
      crsfIndex = 0;
      crsfState = CRSF_STATE_TYPE;
      break;

    case CRSF_STATE_TYPE:
      crsfType = b;
      crsfState = CRSF_STATE_PAYLOAD;
      break;

    case CRSF_STATE_PAYLOAD:
      if (crsfIndex < sizeof(crsfPayload)) {
        crsfPayload[crsfIndex++] = b;
      }
      if (crsfIndex >= (crsfLen - 2)) {  // len = type + payload + crc
        crsfState = CRSF_STATE_CRC;
      }
      break;

    case CRSF_STATE_CRC:
      // CRC wordt hier niet gecheckt (simpel gehouden)
      if (crsfType == CRSF_TYPE_CHANNELS && crsfIndex >= 22) {
        // 16 kanalen, 11-bit packed in 22 bytes
        uint32_t bitOfs = 0;
        for (int ch = 0; ch < 16; ch++) {
          uint32_t byteIndex = bitOfs / 8;
          uint32_t bitIndex = bitOfs % 8;

          uint32_t value = crsfPayload[byteIndex]
                           | (crsfPayload[byteIndex + 1] << 8)
                           | (crsfPayload[byteIndex + 2] << 16);
          value >>= bitIndex;
          value &= 0x7FF;  // 11 bits

          crsfChannels[ch] = (uint16_t)value;
          bitOfs += 11;
        }
      }
      crsfState = CRSF_STATE_SYNC;
      break;
  }
}

void pollCRSF() {
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    processCRSFByte(b);
  }
}

// HID joystick report
typedef struct __attribute__((packed)) {
  uint8_t buttons[4];  // 4 × 8 bits = 32
  int16_t axis[8];
} HIDReport;


HIDReport reportData;

// HID device
Adafruit_USBD_HID hid;

//Switches demux translation
int translation[2000];
int bordervalue[26] = {
  192,
  233,
  324,
  414,
  455,
  545,
  635,
  676,
  713,
  750,
  791,
  881,
  971,
  1011,
  1101,
  1191,
  1232,
  1269,
  1307,
  1347,
  1437,
  1527,
  1568,
  1658,
  1749,
  1789,
};

int swlow[27];
int swmid[27];
int swhigh[27];


uint8_t const hidReportDescriptor[] = {
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x05,  // Usage (Gamepad)
  0xA1, 0x01,  // Collection (Application)

  // -------------------------
  // 32 Buttons
  // -------------------------
  0x05, 0x09,  // Usage Page (Button)
  0x19, 0x01,  // Usage Min = Button 1
  0x29, 0x20,  // Usage Max = Button 32  (0x20 = 32)
  0x15, 0x00,  // Logical Min = 0
  0x25, 0x01,  // Logical Max = 1
  0x75, 0x01,  // Report Size = 1 bit
  0x95, 0x20,  // Report Count = 32 bits (32 buttons)
  0x81, 0x02,  // Input (Data,Var,Abs)



  // -------------------------
  // 8 Axes (16-bit)
  // -------------------------
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x30,  // X
  0x09, 0x31,  // Y
  0x09, 0x32,  // Z
  0x09, 0x33,  // Rx
  0x09, 0x34,  // Ry
  0x09, 0x35,  // Rz
  0x09, 0x36,  // Slider 1
  0x09, 0x37,  // Slider 2

  0x16, 0x00, 0x80,  // Logical Min = -32768
  0x26, 0xFF, 0x7F,  // Logical Max = 32767
  0x75, 0x10,        // Report Size = 16 bits
  0x95, 0x08,        // Report Count = 8 axes
  0x81, 0x02,        // Input (Data,Var,Abs)

  0xC0  // End Collection
};


void setup() {
  Serial2.setRX(CRSF_RX_PIN);
  Serial2.begin(CRSF_BAUD, SERIAL_8N1);

  // HID init
  TinyUSBDevice.setManufacturerDescriptor("RCEJ");
  TinyUSBDevice.setProductDescriptor("CRSF Joystick 8A4M");
  TinyUSBDevice.setSerialDescriptor("CRSF-2604");
  TinyUSBDevice.setID(31280, 30084);

  hid.setReportDescriptor(hidReportDescriptor, sizeof(hidReportDescriptor));
  hid.begin();

    // fill translationtable to convert multiplexed switches
  int y = 0;
  for (int i = 0; i < 1999; i++) {
    if (i > bordervalue[y]) y++;
    translation[i] = y;
  }
  int i = 0;
  for (int s1 = 0; s1 < 3; s1++) {
    for (int s2 = 0; s2 < 3; s2++) {
      for (int s3 = 0; s3 < 3; s3++) {
        if (s1 == 0) swlow[i] += 1;
        if (s1 == 1) swmid[i] += 1;
        if (s1 == 2) swhigh[i] += 1;

        if (s2 == 0) swlow[i] += 2;
        if (s2 == 1) swmid[i] += 2;
        if (s2 == 2) swhigh[i] += 2;

        if (s3 == 0) swlow[i] += 4;
        if (s3 == 1) swmid[i] += 4;
        if (s3 == 2) swhigh[i] += 4;

        i++;
      }
    }
  }
  delay(100);
}

void setup1() {
  delay(500);
}

void loop() {
  pollCRSF();
}

void loop1() {
  static unsigned long lastupdatetime = millis();

  if (millis() - lastupdatetime < 25) {
    // Fast update; only 4 axis
    for (int i = 0; i < 4; i++) {
      reportData.axis[i] = map(crsfChannels[i], 172, 1811, -32768, 32767);
    }

  } else {
    // Complete update; 8 axis and all switches
    for (int i = 0; i < 8; i++) {
      reportData.axis[i] = map(crsfChannels[i], 172, 1811, -32768, 32767);
    }

    // 6 times 2 position switches. Multiplexed with 68%, 27% and 3%
    reportData.buttons[0] = swhigh[translation[crsfChannels[8]]] + 8 * swhigh[translation[crsfChannels[9]]];

    // 6 times 3 position switches
    int tabelindex_swGrp1 = translation[crsfChannels[10]];
    int tabelindex_swGrp2 = translation[crsfChannels[11]];

    reportData.buttons[1] = swlow[tabelindex_swGrp1] + 8 * swlow[tabelindex_swGrp2];
    reportData.buttons[2] = swhigh[tabelindex_swGrp1] + 8 * swhigh[tabelindex_swGrp2];
    reportData.buttons[3] = swmid[tabelindex_swGrp1] + 8 * swmid[tabelindex_swGrp2];

    lastupdatetime = millis();
  }

  if (hid.ready()) {
    hid.sendReport(0, &reportData, sizeof(reportData));
  }
}
