# Operation

See the [`LoggerController`](/docs/commands.md#LoggerController), [`StepperLoggerComponent`](/docs/commands.md#StepperLoggerComponent), and [`OpticalDensityLoggerComponent`](/docs/commands.md#OpticalDensityLoggerComponent) sections of the [`commands.md`](/docs/commands.md) file for the available ministat commands.

# Parts

## General

 - 6mm long M3 rounded head machine screws (e.g. McMaster-Carr [92000A116](https://www.mcmaster.com/92000A116/))
 - 8mm long M3 flat head machine screws (e.g. McMaster-Carr [92010A118](https://www.mcmaster.com/92010A118/))
 - 10mm long M3 rounded head machine screws (e.g. McMaster-Carr [92000A120](https://www.mcmaster.com/92000A120/))
 - 12mm long M3 flat head machine screws (e.g. McMaster [92010A122](https://www.mcmaster.com/92010A122/))
 - 14mm long M3 rounded head machine screws (e.g. McMaster-Carr [92000A124](https://www.mcmaster.com/92000A124/))
 - 18mm long M3 flat head machine screws (e.g. McMaster [92010A788](https://www.mcmaster.com/92010A788/))
 - 22mm long M3 flat head machine screws (e.g.  McMaster [92010A851](https://www.mcmaster.com/92010A851/))
 - 2.4mm thick, 5.5mm across M3 hex nuts (e.g. McMaster-Carr [91828A211](https://www.mcmaster.com/91828A211/))
 - 30mm long M3 aluminum hex standoffs (e.g. McMaster [95947A060](https://www.mcmaster.com/catalog/95947A060))
 - 10mm long M3 nylon hex standoffs (e.g. Amazon [B016ENW3YC](https://www.amazon.com/Uxcell-a15062200ux0544-Spacer-Standoff-Pillar/dp/B016ENW3YC))
 - female-to-male 10cm long ribbon wire (FIXME --> digikey)
 - super glue (e.g. Amazon [B003Y49R7G](https://www.amazon.com/gp/product/B003Y49R7G))

## Wiring

 - 6p4c crimps
 - crimper
for stirrer only
 - 4p4c telephone wire
 - telephone cable union
stirrer and OD reader
 - 10p10c cable (FIXME)

## Boards

 - TODO: update with digi-key parts
 - single row male pin headers (e.g. Amazon [B06XR8CV8P](https://www.amazon.com/Hotop-Pack-Single-Header-Connector/dp/B06XR8CV8P))
 - single row female pin headers (1x12, 1x8, 1x4)
 - double row female pin headers (2x4)


### Photon board

 - TODO: update with digi-key parts
 - 1x (100µF 16V capacitor)[https://www.digikey.com/en/products/detail/nichicon/UPM1C101MED1TD/4319555] (47µF 12V should suffice but to be on the safe side)
 - 1x 10p10c RJ50 ethernet PCB connector (FIXME)


 The pulldown resistors for the signal lines (A0 and A1) should be **10kOhm**. Lower resistances down to 1kOhm work too and change the effective range of the light sensor (with 5V powering the sensor, ~3.8 V max intensity with 1kOhm pull down an 4.1 V max intensity with 10kOhm pulldown) but all are still solidly above the 3.3V max read of the input pins. The pulldown to ground reduces signal for unconnected sensors to just a few mV for all of these resistors and current to ground to < 4mA (best for 10kOhm, worst for 1kOhm: 4V / 1kOhm = 4mA, 4V / 10kOhm = 0.4mA).




### Stepper driver

 - 1x DRV8834 low-voltage stepper motor driver (Pololu [2134](https://www.pololu.com/product/2134))

Measure the reference voltage between logic ground and the tiny trimpot on the DRV8834. By default this will likely be set too high (~1V) which would destroy the stepper motor. Adjust the reference voltage carefully by slowly turning the trimpot counter clockwise using a tiny flathead screwdriver (attach the voltmeter lead to the screw driver) until the reference voltage is between 190mV and 200mV (do not go higher). This limits the current to 400mA, which is the max current supported by the [stepper motor](https://www.sparkfun.com/products/10551) used here. See [this video](https://youtu.be/89BHS9hfSUk) for details on this current limiting stepper motor driver.

## Controller Box

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
 - 3x 30mm long M3 aluminum hex standoffs as base feet
 - 3x 10mm long M3 rounded head machine screws to attach the base feet
 Option 1 (motor flush with bottom):
 - 2x 10mm long M3 nylon hex standoffs to attach motor
 - 2x 6mm long M3 rounded head machine screws to attach motor from bottom (5mm works too: [92000A114](https://www.mcmaster.com/92000A114/))
 - 2x 8mm long M3 flat head machine screws to attach motor from top
 Option 2 (motor as high as possible):
 - 2x 18mm long M3 flat head machine screws to attach motor from top with 10mm 3D printed spacer ring
optiona:
 - 4x 6mm long M3 rounded head machine screws to attach size adapter ring to stirrer base

Cut off the connector at the end of the motor cables with a wire cutter and crimp a telephone cable wire crimp (6p4c) on the end in its stead. Be careful about wire order. Looking from the back of the connector (where the wires go in) with the connector clip facing up, the order should be *yellow, orange, brown, black* from left to right.

## OD attachment

 - 6x 14mm long M3 rounded head machine screws to attach the LED and sensors
Either
 - 4x 18mm long M3 flat head machine screw to attach optical density reader to stirrer base
OR
 - 4x 22mm long M3 flat head machine screw to attach OD reader AND a size adapter ring to stirrer base

# Implementatoin Notes

## Optics

### Beam splitter

The microscope cover slip acts as a simple beam splitter that splits the LED beam ~9:1 (transmitted:reflected). The transmitted light path is horizontally shifted from its incident light path:

$$
x = d\sin\theta\left(1-\frac{\sqrt{1-\sin^2\theta}}{\sqrt{n^2-\sin^2\theta}}\right)
$$

where x is the shift, d is the thickness of the cover slip, $\theta$ is the incident angle of the light and n is the refractive index of the glass (assuming air is 1). With $\theta = 45$ degrees and $n=1.52$, $x = 0.34*d$. For the standard thickness of a cover slip (0.15mm), this shift is only 0.05 mm so is not corrected for in the design of the light path but for a thicker beam splitter it would have to be.

### Focusing effects

 Round bottles with water act as a convex lens focusing light in the horizontal and thus increasing the light intensity depending on the bottle shape, exact position, etc. - this could be addressed with a separate focusing lens in front of the sensor but would raise the cost and complicate the 3D printed design. Instead for now, key is to mark the orientation of the media bottle carefully to make sure focusing remains the same throughout the experiment (ideally leave the bottle in the same position at all times).