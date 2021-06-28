# Dallas DS18B20

The Dallas DS18B20 is a digital 1-wire thermometer with accuracy of 0.5C in a temperature range from -10C to +85C. For details on the chip, see [this spec sheet](https://cdn-shop.adafruit.com/datasheets/DS18B20.pdf).

For lab use, the encased and waterproof version of this sensor [sold by Adafruit](https://www.adafruit.com/product/381) (and other places). Is recommended.

## Wiring of the encased sensor

For the encased sensor, the red wire connects to 3-5V, the black to ground, and the white/yellow is the 1-wire data line. With the lablogger particle photon PCB, that means the black wire should connect to pin 1, red wire to pin 2 and white wire to any of pins 3-8 of an ethernet jack (usually pin 3). The data pin (e.g. 3) then just needs to connect to the relevant 1-wire pin on the PCB (A0 by default) and have a 4.7 kOhm pullup resistor connecting the data pin to 3-5V.

# AM2315

To additionally get humidity sensing, The [AM2315](https://www.adafruit.com/product/1293) combines a DS18B20 temperature sensor with a humidity sensor in a sigle casing. It's not quite waterproof but still pretty resilient for lab use if humidity sensing is required.