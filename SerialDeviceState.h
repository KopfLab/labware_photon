#pragma once
#include "device/DeviceState.h"
#include "device/SerialDeviceCommands.h"

// constants
#define READ_MANUAL   0 // data reading is manual

// scale state
struct SerialDeviceState : public DeviceState {

  unsigned int data_reading_period_min; // minimum time between reads (in ms)
  unsigned int data_reading_period;     // period between reads (stored in ms!!!)

  SerialDeviceState () {};

  SerialDeviceState (bool locked, bool state_logging, bool data_logging, unsigned int data_reading_period_min, unsigned int data_reading_period, unsigned int data_logging_period, int data_logging_type) :
    DeviceState(locked, state_logging, data_logging, data_logging_period, data_logging_type), data_reading_period_min(data_reading_period_min), data_reading_period(data_reading_period) {};
};

/**** textual translations of state values ****/

// logging_period (any pattern)
static void getStateDataReadingPeriodText(unsigned int reading_period, char* target, int size, const char* pattern, bool include_key = true) {
  if (reading_period == 0) {
    // manual mode
    getStateStringText(CMD_DATA_READ_PERIOD, CMD_DATA_READ_PERIOD_MANUAL, target, size, pattern, include_key);
  } else {
    // specific reading period
    char units[] = "ms";
    if (reading_period % 60000 == 0) {
      // minutes
      strcpy(units, "m");
      reading_period = reading_period/60000;
    } else if (reading_period % 1000 == 0) {
      // seconds
      strcpy(units, "s");
      reading_period = reading_period/1000;
    }
    getStateIntText(CMD_DATA_READ_PERIOD, reading_period, units, target, size, pattern, include_key);
  }
}

// read period (standard patterns)
static void getStateDataReadingPeriodText(unsigned int reading_period, char* target, int size, bool value_only = false) {
  if (value_only) {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_V_SIMPLE, false) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_VU_SIMPLE, false); // number
  } else {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KV_JSON_QUOTED, true) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KVU_JSON, true); // number
  }
}
