/*
 * LoggerControllerState.h
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 * Structure that captures the Logger state. Note that this is trouble as a class or a structure
 * with methods because of issues in EEPROM storage. Methods that modify and save/restore states
 * are implemented in LoggerController.h and derived classes instead.
 */

#pragma once
#include "LoggerCommand.h"
#include "LoggerConstants.h"

// log types
#define LOG_BY_TIME    0 // log period is a time in seconds
#define LOG_BY_EVENT   1 // log period is a number (x times)

// mnaual read constant
#define READ_MANUAL    0 // data reading is manual

struct LoggerControllerState {
  bool locked = false; // whether state is locked
  bool state_logging = false; // whether state is logged (whenever there is a change)
  bool data_logging = false; // whether data is logged
  uint data_logging_period ; // period between logs (in seconds!)
  uint8_t data_logging_type; // what the data logging period signifies
  bool data_reader = false; // whether this controller is a data reader
  uint data_reading_period_min; // minimum time between reads (in ms) [only relevant if it is a data_reader]
  uint data_reading_period; // period between reads (stored in ms!!!) [only relevant if it is a data_reader]
  uint8_t version = 3;

  LoggerControllerState() {};
  // without data reading settings
  LoggerControllerState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type) :
    locked(locked), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(false)  {}
  // with data reading settings
  LoggerControllerState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, uint data_reading_period_min, uint data_reading_period) :
    locked(locked), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(true), data_reading_period_min(data_reading_period_min), data_reading_period(data_reading_period)  {}

};

/**** textual translations of state values ****/

// locked text
static void getStateLockedText(bool locked, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_LOCK, locked, CMD_LOCK_ON, CMD_LOCK_OFF, target, size, pattern, include_key);
}
static void getStateLockedText(bool locked, char* target, int size, bool value_only = false) {
  if (value_only) getStateLockedText(locked, target, size, PATTERN_V_SIMPLE, false);
  else getStateLockedText(locked, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// state logging
static void getStateStateLoggingText(bool state_logging, char* target, int size, char* pattern, bool include_key = true) {
  #ifdef WEBHOOKS_DEBUG_ON
    getStateStringText(CMD_STATE_LOG, "debug", target, size, pattern, include_key);
  #else
    getStateBooleanText(CMD_STATE_LOG, state_logging, CMD_STATE_LOG_ON, CMD_STATE_LOG_OFF, target, size, pattern, include_key);
  #endif
}

static void getStateStateLoggingText(bool state_logging, char* target, int size, bool value_only = false) {
  if (value_only) getStateStateLoggingText(state_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateStateLoggingText(state_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// data logging
static void getStateDataLoggingText(bool data_logging, char* target, int size, char* pattern, bool include_key = true) {
  #ifdef WEBHOOKS_DEBUG_ON
    getStateStringText(CMD_DATA_LOG, "debug", target, size, pattern, include_key);
  #else
    getStateBooleanText(CMD_DATA_LOG, data_logging, CMD_DATA_LOG_ON, CMD_DATA_LOG_OFF, target, size, pattern, include_key);
  #endif
}

static void getStateDataLoggingText(bool data_logging, char* target, int size, bool value_only = false) {
  if (value_only) getStateDataLoggingText(data_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateDataLoggingText(data_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// data logging period (any pattern)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, char* pattern, bool include_key = true) {
  // specific logging period
  char units[] = "?";
  if (logging_type == LOG_BY_EVENT) {
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

// logging period (standard patterns)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, bool value_only = false) {
  if (value_only) {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_VU_SIMPLE, false);
  } else {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_KVU_JSON, true);
  }
}

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
