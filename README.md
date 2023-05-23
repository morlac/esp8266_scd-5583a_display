= esp8266_scd-5583a_display =

== ESP8266 - NodeMCU3 pinout ==

/RST - scd5583 1 2: /RST

  D1 - i2c: SCL
  D2 - i2c: SDA

[..]

V1:
  D5 - scd5583 1 2: DATA
  D6 - scd5583 1 2: SDCLK
  D7 - scd5583 1  : /LOAD
  D8 - scd5583   2: /LOAD

V2:
  D5 - scd5583   2: /LOAD
  D6 - scd5583 1 2: SDCLK
  D7 - scd5583 1 2: DATA
  D8 - scd5583 1  : /LOAD

== scd5583a - pinout ==

  1 - SDCLK        GND - 28
  2 - /LOAD       DATA - 27
  3 - nc            nc - 26
  4 - nc            nc - 25
  5 - nc            nc - 24
  6 - nc            nc - 23
  7 - nc            nc - 22
  8 - nc            nc - 21
  9 - nc            nc - 20
 10 - nc           VCC - 19
 11 - nc            na - 18
 12 - nc            nc - 17
 13 - /RST     /CLKSEL - 16
 14 - GND      CLK I/O - 15


