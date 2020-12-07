# Lablogger devices

Microcontrollers for lablogger.

## Makefile

- to compile: make PROGRAM 
- to flash latest compile via USB: make flash
- to flash latest compile via cloud: make flash device=DEVICE
- to start serial monitor: make monitor
- to compile & flash: make PROGRAM flash
- to compile, flash & monitor: make PROGRAM flash monitor

## Available programs

 - fill in

# revise the following

## Usage

This repository could at some point be turned into a particle photon library but is too lab specific at the moment.
To include as a submodule in labware projects:

- `cd` to the other labware project, there `git submodule add https://github.com/KopfLab/labware_photon device`
- to update most easily, from project main directory (above submodule): `git submodule update --remote`
- to check out the project elsewhere `git submodule update --init --recursive` and then in the folder `git checkout master` and `git pull`

Then reference to the `labware_photon` classes via:

- `#include "device/Device???.h"`

## Serial

The null soft RS232 module used typically in our serial device housings is described in detail at http://www.nulsom.com/datasheet/NS-RS232_en.pdf. The important pins on the DB9 connector are 2 (RX), 3 (TX) and 5 (GND).

## Web commands

To run any web commands, you need to either have the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli) installed, or format the appropriate POST request to the [Particle Cloud API](https://docs.particle.io/reference/api/). Here only the currently implemented CLI calls are listed but they translate directly into the corresponding API requests. You only have access to the photons that are registered to your account.

### requesting information via CLI

The state of the device can be requested by calling `particle get <deviceID> device_state` where `<deviceID>` is the name of the photon you want to get state information from. The latest data can be requested by calling `particle get <deviceID> device_data`. The return values are always array strings (ready to be JSON parsed) that include information on the state or last registered data of the device, respectively. Requires being logged in (`particle login`) to have access to the photons.

### issuing commands via CLI

All calls are issued from the terminal and have the format `particle call <deviceID> device "<cmd>"` where `<deviceID>` is the name of the photon you want to issue a command to and `<cmd>` is the command (and should always be in quotes) - e.g. `particle call my-logger device "data-log on"`. If the command was successfully received and executed, `0` is returned, if the command was received but caused an error, a negative number (e.g. `-1` for generic error, `-2` for device is locked, `-3` for unknown command, etc.) is the return value. Positive return values mean executed with warning (e.g. `1` to note that the command did not change anything). The command's exact wording and all the return codes are defined in `DeviceCommands.h` and `SerialDeviceCommands.h`. Issuing commands also requires being logged in (`particle login`) to have access to the photons.

Some common state variables are displayed in short notation in the upper right corner of the LCD screen (same line as the device name) - called **state overview**. It is noted in the following command lists where this is the case.

#### `DeviceController` commands (`<cmd>` options):

  - `state-log on` to turn web logging of state changes on (letter `S` shown in the state overview)
  - `state-log off` to turn web logging of state changes off (no letter `S` in state overview)
  - `data-log on` to turn web logging of data on (letter `D` in state overview)
  - `data-log off` to turn web logging of data off
  - `lock on` to safely lock the device (i.e. no commands will be accepted until `lock off` is called) - letter `L` in state overview
  - `lock off` to unlock the device if it is locked
  - `reset data` to reset the data stored in the device

#### Additional `SerialDeviceController` commands:

  - `read-period <options>` to specify how frequently data should be read (letter `R` + subsequent in state overview), `<options>`:
    - `manual` don't read data unless externally triggered in some way (device specific) - `RM` in state overview
    - `200 ms` read data every 200 (or any other number) milli seconds (`R200ms` in state overview)
    - `5 s` read data every 5 (or any other number) seconds (`R5s` in state overview)
  - `log-period <options>` to specify how frequently read data should be logged (after letter `D` in state overview, although the `D` only appears if data logging is actually enabled), `<options>`:
    - `3 x` log after every 3rd (or any other number) successful data read (`D3x` in state overview if logging is active, just `3x` if not), works with `manual` or time based `read-period`, set to `1 x` in combination with `manual` to log every externally triggered data event immediately
    - `2 s` log every 2 seconds (or any other number), must exceed the `read-period` (`D2s` in state overview if data logging is active, just `2s` if not)
    - `8 m` log every 8 minutes (or any other number)
    - `1 h` log every hour (or any other number)
