# Photon labware

Base classes for particle photons tied into the lab device and data infrastructure.

## Usage

This repository could at some point be turned into a particle photon library but is too lab specific at the moment.
To include as a submodule in labware projects:

- `cd` to the other labware project, there `git submodule add https://github.com/KopfLab/labware_photon device`
- to update (or when someone else is checking out the project for) `git submodule update --init --recursive`

Then reference to the `labware_photon` classes via:

- `#include "device/Device???.h"`

## Web commands

To run any web commands, you need to either have the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli) installed, or format the appropriate POST request to the [Particle Cloud API](https://docs.particle.io/reference/api/). Here only the currently implemented CLI calls are listed but they translate directly into the corresponding API requests. You only have access to the photons that are registered to your account.

### requesting information via CLI

The state of the device can be requested by calling `particle get <deviceID> device_state` where `<deviceID>` is the name of the photon you want to get state information from. The latest data can be requested by calling `particle get <deviceID> device_data`. The return values are always array strings (ready to be JSON parsed) that include information on the state or last registered data of the device, respectively. Requires being logged in (`particle login`) to have access to the photons.

### issuing commands via CLI

All calls are issued from the terminal and have the format `particle call <deviceID> device "<cmd>"` where `<deviceID>` is the name of the photon you want to issue a command to and `<cmd>` is the command (and should always be in quotes) - e.g. `particle call my-logger device "data-log on"`. If the command was successfully received and executed, `0` is returned, if the command was received but caused an error, a negative number (e.g. `-1` for generic error, `-2` for device is locked, `-3` for unknown command, etc.) is the return value. Positive return values mean executed with warning (e.g. `1` to note that the command did not change anything). The command's exact wording and all the return codes are defined in `DeviceCommands.h` and `DeviceCommandsLogger.h`. Issuing commands also requires being logged in (`particle login`) to have access to the photons.

#### `DeviceController` commands (`<cmd>` options):

  - `state-log on` to turn web logging of state changes on
  - `state-log off` to turn web logging of state changes off
  - `data-log on` to turn web logging of data on
  - `data-log off` to turn web logging of data off
  - `lock on` to safely lock the device (i.e. no commands will be accepted until `lock off` is called)
  - `lock off` to unlock the device if it is locked

#### Additional `DeviceControllerDataLogger` commands:

  - `read-period <options>` to specify how frequently data should be read, `<options>`:
    - `manual` don't read data unless externally triggered in some way (device specific)
    - `200 ms` read data every 200 (or any other number) milli seconds
    - `5 s` read data every 5 (or any other number) seconds
  - `log-period <options>` to specify how frequently read data should be logged, `<options>`:
    - `3 x` log after every 3rd (or any other number) successful data read, works with `manual` or time based `read-period`, set to `1 x` in combination with `manual` to log every externally triggered data event immediately
    - `2 s` log every 2 seconds (or any other number), must exceed the `read-period`
    - `8 m` log every 8 minutes (or any other number)
    - `1 h` log every hour (or any other number)

#### Additional `DeviceControllerSerialLogger` commands:
