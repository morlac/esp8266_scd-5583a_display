#ifndef _SCD5583_H_
#define _SCD5583_H_

#include  <Arduino.h>

class SCD5583 {
  public:
    SCD5583(byte loadPin, byte dataPin, byte clkPin);
    
    /**
     * clears the complete Matrix
     */
    void clearMatrix(void);

    /**
     * set Brightness of Matrix to level:
     * 
     * 0 - 100%
     * 1 -  53%
     * 2 -  40%
     * 3 -  27%
     * 4 -  20%
     * 5 -  13%
     * 6 -   6.6%
     * 7 -   0%
     */
    void setBrightness(byte b);

    /**
     * 
     */
    uint8_t getBrightness(void);

    /**
     * write line of up to 8 chars to display 
     */
    void writeLine(char text[9]);

  private:
    void _sendData(byte data);
    void _getCharDefinition(byte* rows, char c);
    void _selectIndex(byte position);

    byte _loadPin;
    byte _dataPin;
    byte _clkPin;

    byte set_brightness;
};

#endif // _SCD5583_H_
