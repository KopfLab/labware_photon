# Lablogger devices

Microcontrollers for lablogger.

## Features

- easily extensible framework for implementing cloud-connected instrument controllers and data logging devices based on the secure and well-established Particle Photon platform
- flexible components for data reading, serial communication, stepper motor control, etc. that can be combined into a single controller as needed
- logging framework constructs JSON-formatted data logs for flexible recording in spreadsheets or databases via cloud webhooks
- build-in data averaging and error calculation
- built-in support for remote control via cloud commands
- built-in support for device state management (device locking, logging behavior, data read and log frequency, etc.)
- built-in connectivity management with data cashing during offline periods - Photon memory typically allows cashing of 50-100 logs to bridge device downtime of several hours

## Makefile

- to compile: make PROGRAM 
- to flash latest compile via USB: make flash
- to flash latest compile via cloud: make flash device=DEVICE
- to start serial monitor: make monitor
- to compile & flash: make PROGRAM flash
- to compile, flash & monitor: make PROGRAM flash monitor

## Available programs

### devices

 - `devices/ministat`: ministat controller including a stepper component for stirring and an OD reader component for recording optical density 
 - `devices/chemglass_scale` : logger for chemglass scales (pulls weight data via serial and calculates rates on the fly)

### debug

 - `debug/blink`: simple blink program to check if photon works
 - `debug/cloud`: use to debug wifi settings and cloud connection
 - `debug/i2c_scanner`: use to search for the address(es) of I2C connected devices
 - `debug/lcd`: use debug I2C-connected LCD screens
 - `debug/logger`: use to test out a basic lab logger setup with an example component

# Web commands

To run any web commands, you need to either have the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli) installed, or format the appropriate POST request to the [Particle Cloud API](https://docs.particle.io/reference/api/). Here only the currently implemented CLI calls are listed but they translate directly into the corresponding API requests. You only have access to the photons that are registered to your account.

## requesting information via CLI

The state of the device can be requested by calling `particle get <deviceID> state` where `<deviceID>` is the name of the photon you want to get state information from. The latest data can be requested by calling `particle get <deviceID> data`. The return values are always array strings (ready to be JSON parsed) that include information on the state or last registered data of the device, respectively. Requires being logged in (`particle login`) to have access to the photons.

## issuing commands via CLI

All calls are issued from the terminal and have the format `particle call <deviceID> device "<cmd>"` where `<deviceID>` is the name of the photon you want to issue a command to and `<cmd>` is the command (and should always be in quotes) - e.g. `particle call my-logger device "data-log on"`. If the command was successfully received and executed, `0` is returned, if the command was received but caused an error, a negative number (e.g. `-1` for generic error, `-2` for device is locked, `-3` for unknown command, etc.) is the return value. Positive return values mean executed with warning (e.g. `1` to note that the command did not change anything). The command's exact wording and all the return codes are defined in the header files of the controller and components (e.g. `LoggerController.h` and `ExampleLoggerComponent.h`). Issuing commands also requires being logged in (`particle login`) to have access to the photons.

Some common state variables are displayed in short notation in the upper right corner of the LCD screen (same line as the device name) - called **state overview**. The state overview starts with a `W` if the photon has internet connection and `!` if it currently does not (yet). Additoinal letters and their meanings are noted in the following command lists when applicable.

## available commands

See the full documentation [here](docs/commands.md) for a list of available commands from the contorller and various components.

# Troubleshooting

## Photon reset

To reset a photon, use `particle device doctor` (or `make doctor`) to flash new firmware (first step) and you can reset other parts as well but setting new WiFi credentials does not always work this way. Instead afterwards modify the `debug/credentials` app with the correct wifi credentials and flash it with `make debug/credentials flash monitor`. If there are connectivity issues, flash `make debug/cloud flash monitor` to check wifi configuration. Note that if the photon is not claimed yet (or claimed to a different account), you always still need to hold `SETUP` until blue blinking and run `particle setup` from the command line to claim the photon (you have to set wifi credentials again during setup but can overwrite with desired using `make debug/credentials`). Note that the `particle setup` configuration of the wifi settings does not always work so it is easier to just use `debug/credentials`.