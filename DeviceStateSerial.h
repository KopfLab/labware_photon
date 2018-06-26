#pragma once
#include "device/DeviceState.h"
#include "device/DeviceCommandsSerial.h"

#define LOG_BY_TIME   0 // log period is a time in seconds
#define LOG_BY_READ   1 // log period is a number (x times)
#define READ_MANUAL   0 // data reading is manual

// scale state
struct DeviceStateSerial : public DeviceState {

  uint data_reading_period_min; // minimum time between reads (in ms)
  uint data_reading_period;     // period between reads (stored in ms!!!)

  uint data_logging_period; // period between logs (stored in s!!!)
  uint8_t data_logging_type; // whether the data logging period is a time or number (use int to have it be expandable)

  DeviceStateSerial () {};

  DeviceStateSerial (bool locked, bool state_logging, bool data_logging, uint data_reading_period_min, uint data_reading_period, uint data_logging_period, uint8_t data_logging_type) :
    DeviceState(locked, state_logging, data_logging), data_reading_period_min(data_reading_period_min), data_reading_period(data_reading_period), data_logging_period(data_logging_period), data_logging_type(data_logging_type) {};
};

/**** textual translations of state values ****/

// logging_period (any pattern)
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, char* pattern, bool include_key = true) {
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
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, bool value_only = false) {
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

// logging_period (any pattern)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, char* pattern, bool include_key = true) {
  // specific logging period
  char units[] = "?";
  if (logging_type == LOG_BY_READ) {
    // by read
    strcpy(units, CMD_DATA_LOG_PERIOD_NUMBER);
  } else  if (logging_type == LOG_BY_TIME) {
    // by time
    if (logging_period % 3600 == 0) {
      // hours
      strcpy(units, "h");
      logging_period = logging_period/3600;
    } else if (logging_period % 60 == 0) {
      // minutes
      strcpy(units, "m");
      logging_period = logging_period/60;

    } else {
      strcpy(units, "s");
    }
  } else {
    // not supported!
    Serial.printf("ERROR: unknown logging type %d\n", logging_type);
  }

  getStateIntText(CMD_DATA_LOG_PERIOD, logging_period, units, target, size, pattern, include_key);
}

// read period (standard patterns)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, bool value_only = false) {
  if (value_only) {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_VU_SIMPLE, false);
  } else {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_KVU_JSON, true);
  }
}
