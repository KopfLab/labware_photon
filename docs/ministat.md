


# Parts

## General

 - 6mm long M3 rounded head machine screws (e.g. McMaster-Carr [92000A116](https://www.mcmaster.com/92000A116/))
 - 10mm long M3 rounded head machine screws (e.g. McMaster-Carr [92000A120](https://www.mcmaster.com/92000A120/))
 - 8mm long M3 flat head machine screws (e.g. McMaster-Carr [92010A118](https://www.mcmaster.com/92010A118/))
 - 12mm long M3 flat head machine screws (e.g. McMaster [92010A122](https://www.mcmaster.com/92010A122/))
 - 2.4mm thick, 5.5mm across M3 hex nuts (e.g. McMaster-Carr [91828A211](https://www.mcmaster.com/91828A211/))
 - 30mm long M3 aluminum hex standoffs (e.g. McMaster [95947A060](https://www.mcmaster.com/catalog/95947A060))
 - 10mm long M3 nylon hex standoffs (e.g. Amazon [B07FZGG76T](https://www.amazon.com/M3x10mm-Female-Thread-Standoff-Spacer/dp/B07FZGG76T))
 - female-to-male 10cm long ribbon wire
 - liquid glue (e.g. Elmer's clear washable glue)

## Wiring

 - 6p4c crimps
 - cripmer
 - 4p4c telephone wire
 - telephone cable union

## Boards

 - single row male pin headers (e.g. Amazon [B06XR8CV8P](https://www.amazon.com/Hotop-Pack-Single-Header-Connector/dp/B06XR8CV8P))
 - single row female pin headers (1x12, 1x8, 1x4)
 - double row female pin headers (2x4)


### Photon board

 - 1x 100µF 16V capacitor (47µF 12V should suffice but to be on the safe side)

### Stepper driver

 - 1x DRV8834 low-voltage stepper motor driver (Pololu [2134](https://www.pololu.com/product/2134))

Measure the reference voltage between logic ground and the tiny trimpot on the DRV8834. By default this will likely be set too high (~1V) which would destroy the stepper motor. Adjust the reference voltage carefully by slowly turning the trimpot counter clockwise using a tiny flathead screwdriver (attach the voltmeter lead to the screw driver) until the reference voltage is between 190mV and 200mV (do not go higher). This limits the current to 400mA, which is the max current supported by the [stepper motor](https://www.sparkfun.com/products/10551) used here. See [this video](https://youtu.be/89BHS9hfSUk) for details on this current limiting stepper motor driver.

## Controller

**back**:
 - RJ11 6P4C telephone cable port to connect motor cable (Amazon [B07KYTMXJN](https://www.amazon.com/gp/product/B07KYTMXJN))
 - 2x 10mm long M3 rounded head screws to attach RJ11 port
 - 4x 10mm long M3 nylon hex standoffs to attach PCB board
 - 4x 6mm long M3 rounded head screws to attach PCB board to standoffs
 - 4x 8mm long M3 flat head screws to attach PCB standoffs to back panel

 Attach RJ11 cable to the motor out (A-, A+, B-, B+) with the red cable closest to the screw hole on the PCB. If this is reversed, the motor will work but direction (clockwise vs. counter clockwise) will be reversed.


**front**:
 - 1x 16x2 LCD screen wih I2C backpack
 - 4x 12mm long M3 flat head machine screws to attach LCD screen
 - 4x M3 hex nuts to attach LCD screen

 **assembly**:
 - 8x 8mm long M3 flat head machine screws to attach front and back to housing
 - 8x M3 hex nuts to attach front and back to housing


## Base with stirrer
 - 1x 3D printed base (custom)
 - 1x ST-PM35-15-11C stepper motor (SparkFun)
 - 2x 10mm long M3 nylon hex standoffs to attach motor
 - 2x 6mm long M3 rounded head machine screws to attach motor from bottom (5mm works too: [92000A114](https://www.mcmaster.com/92000A114/))
 - 2x 8mm long M3 flat head machine screws to attach motor from top
 - 3x 30mm long M3 aluminum hex standoffs as base feet
 - 3x 10mm long M3 rounded head machine screws to attach the base feet

## OD attachment