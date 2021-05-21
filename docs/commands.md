---
title: "Lablogger device commands"
author: "Sebastian Kopf"
output:
  pdf_document: default
  html_document: default
geometry: margin=1in
---

The following commands can be used directly through the [lablogger GUI](https://github.com/KopfLab/lablogger) or take the place of the `<cmd>` placeholder in the CLI call `particle call <deviceID> device "<cmd>"`.

# [`LoggerController`](/src/modules/logger/LoggerController.h) commands:

The following commands are available for all loggers. Addtional commands are provided by individual components listed hereafter.

  - `state-log on` to turn web logging of state changes on (letter `S` shown in the state overview)
  - `state-log off` to turn web logging of state changes off (no letter `S` in state overview)
  - `data-log on` to turn web logging of data on (letter `D` in state overview)
  - `data-log off` to turn web logging of data off
  - `log-period <options>` to specify how frequently data should be logged (after letter `D` in state overview, although the `D` only appears if data logging is actually enabled), `<options>`:
    - `3 x` log after every 3rd (or any other number) successful data read (`D3x`), works with `manual` or time based `read-period`, set to `1 x` in combination with `manual` to log every externally triggered data event immediately (**FIXME**: not fully implemented)
    - `2 s` log every 2 seconds (or any other number), must exceed the `read-period` (`D2s` in state overview)
    - `8 m` log every 8 minutes (or any other number)
    - `1 h` log every hour (or any other number)
  - `read-period <options>` to specify how frequently data should be read (letter `R` + subsequent in state overview), only applicable if the controller is set up to be a data reader, `<options>`:
    - `manual` don't read data unless externally triggered in some way (device specific) - `RM` in state overview
    - `200 ms` read data every 200 (or any other number) milli seconds (`R200ms` in state overview)
    - `5 s` read data every 5 (or any other number) seconds (`R5s` in state overview)
  - `lock on` to safely lock the device (i.e. no commands will be accepted until `lock off` is called) - letter `L` in state overview
  - `lock off` to unlock the device if it is locked
  - `restart` to force a restart
  - `reset state` to completely reset the state back to the default values (forces a restart after reset is complete)
  - `reset data` to reset the data currently being collected
  - `page` to switch to the next page on the LCD screen (**FIXME**: not fully implemented)

# [`ScaleLoggerComponent`](/src/modules/scale/ScaleLoggerComponent.h) commands:

  - all `LoggerController` commands PLUS:
  - `calc-rate <options>` to specify whether to calculate a mass flow rate and if so, which time units to use, `<options>`:
    - `off` don't calculate the flow rate
    - `s` calculate flow rate as mass/s
    - `m` calculate flow rate as mass/minute
    - `h` calculate flow rate as mass/hours
    - `d` calculate flow rate as mass/day

# [`MFCLoggerComponent`](/src/modules/mfc/MFCLoggerComponent.h) commands:

  - all `LoggerController` commands PLUS:
  - `mfc <ID>` to specify the ID of the mass flow controller to communicate with (for Alicat MFCs typically a letter from A-Z)
  - `setpoint <value> <units>` to specify the mass flow setpoint (must be in the units the MFC is set to)
  - `start` start the MFC flow (at the set setpoint)
  - `stop` stop the MFC flow

# [`StepperLoggerComponent`](/src/modules/mfc/StepperLoggerComponent.h) commands:

  - all `LoggerController` commands PLUS:
  - `start` to start the motor (at the currently set speed and microstepping)
  - `stop` to stop the motor and disengage it (no holding torque applied)
  - `hold` to stop the motor but hold the position (maximum holding torque)
  - `rotate <x>` to have the motor do `<x>` rotations and then execute a `stop` commands
  - `ms <x>` to set the microstepping mode to `<x>` (1= full step, 2 = half step, 4 = quarter step, etc.)
  - `ms auto` to set the microstepping mode to automatic in which case the lowest step mode that the current speed allows will be automatically set
  - `speed <x> rpm` to set the motor speed to `<x>` rotations per minute (if the motor is currently running, it will change the speed to this and keep running). if microstepping mode is in `auto` it will automatically select the appropriate microstepping mode for the selected speed. If the microstepping mode is fixed and the requested rpm exceeds the maximally possible speed for the selected mode (or if in `auto` mode, the requested rpm exceeds the fastest possible on full step mode), the maximum speed will automatically be set instead and a warning return code will be issued.
  - `direction cc` to set the direction to counter clockwise
  - `direction cw` to set the direction to clockwise
  - `direction switch` to reverse the direction (note that any direction changes stops the motor if it is in `rotate <x>` mode)
