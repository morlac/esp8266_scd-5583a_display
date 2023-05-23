= esp8266_scd-5583a_display =

== ESP8266 - NodeMCU3 pinout ==

| ESP signal | scd5583 signal | i2c signal |
| ---------- | -------------- | ---------- |
|    /RST    |   1 2: /RST    |            |
|            |                |            |
|     D1     |                |     SCL    |
|     D2     |                |     SDA    |
|            |                |            |
|            |      [..]      |            |
|            |                |            |
|            |       V1:      |            |
|     D5     |   1 2: DATA    |            |
|     D6     |   1 2: SDCLK   |            |
|     D7     |   1  : /LOAD   |            |
|     D8     |     2: /LOAD   |            |
|            |                |            |
|            |       V2:      |            |
|     D5     |     2: /LOAD   |            |
|     D6     |   1 2: SDCLK   |            |
|     D7     |   1 2: DATA    |            |
|     D8     |   1  : /LOAD   |            |

== scd5583a - pinout ==

| pin | signal | - |  signal | pin |
| ---:|:------ | - | -------:|:--- |
|   1 | SDCLK  |   |     GND |  28 |
|   2 | /LOAD  |   |    DATA |  27 |
|   3 | nc     |   |      nc |  26 |
|   4 | nc     |   |      nc |  25 |
|   5 | nc     |   |      nc |  24 |
|   6 | nc     |   |      nc |  23 |
|   7 | nc     |   |      nc |  22 |
|   8 | nc     |   |      nc |  21 |
|   9 | nc     |   |      nc |  20 |
|  10 | nc     |   |     VCC |  19 |
|  11 | nc     |   |      na |  18 |
|  12 | nc     |   |      nc |  17 |
|  13 | /RST   |   | /CLKSEL |  16 |
|  14 | GND    |   | CLK I/O |  15 |


