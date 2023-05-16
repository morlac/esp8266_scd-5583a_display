/**
 * 
 */

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "SCD5583.hpp"
#include "font5x5.h"

#define MAX_CHARS 8   // set to 10 for SCD5510X (10 digits) or to 4 for SCD554X (4 digits);
#define NUM_ROWS  5

/**
 * 
 */
SCD5583::SCD5583(byte loadPin, byte dataPin, byte clkPin) {
  pinMode(loadPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clkPin, OUTPUT);
  
  _loadPin = loadPin;
  _dataPin = dataPin;
  _clkPin  = clkPin;

  set_brightness = 0;

  clearMatrix();
//  setBrightness(0);
}

/**
 * 
 */
void SCD5583::clearMatrix() {
  _sendData(B11000000);
}

/**
 * 0 - 100%
 * 1 -  53%
 * 2 -  40%
 * 3 -  27%
 * 4 -  20%
 * 5 -  13%
 * 6 -   6.6%
 * 7 -   0%
 */
void SCD5583::setBrightness(byte b) {
  set_brightness = b;

  if (b >= 0 && b < 7) {
    _sendData(B11110000 + b);
  } else if (b == 7) {
    _sendData(B11110000 + 0xF);
  }
}

/**
 * 
 */
uint8_t SCD5583::getBrightness(void) {
  return set_brightness;
}

/**
 * 
 */
void SCD5583::_selectIndex(byte position) {
   _sendData(B10100000 + position);
}

/**
 * 
 */
void SCD5583::_sendData(byte data) {
  byte mask = 1;

  digitalWrite(_loadPin, LOW);
  digitalWrite(_clkPin,  LOW);

  for (byte i = 0; i < 8; i++) {
    digitalWrite(_dataPin, (mask & data ? HIGH : LOW));
    digitalWrite(_clkPin, HIGH);
//  delay(10);
    digitalWrite(_clkPin, LOW);
    
    mask = mask*2;
  }

  digitalWrite(_loadPin, HIGH);
}

/**
 * 
 */
void SCD5583::_getCharDefinition(byte* rows, char c) {
  for (byte i = 0; i < NUM_ROWS; i++) {
    rows[i] = pgm_read_byte_near(font5x5 + ((c - 0x20) * NUM_ROWS) + i);
  }
}

/**
 * 
 */
void SCD5583::writeLine(char* text) {
  byte rows[NUM_ROWS];

  for (int i = 0; i < MAX_CHARS; i++) {
    _selectIndex(i);
    _getCharDefinition(rows, text[i]);
    
    for (int r = 0; r < NUM_ROWS; r++) {
      _sendData(rows[r]);
    }
  }
}
