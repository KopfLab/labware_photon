/*
 * DeviceState.h
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 * Structure that captures the device state. Note that this is trouble as a class or a structure
 * with methods because of issues in EEPROM storage. Methods that modify and save/restore states
 * are implemented in DeviceController.h and derived classes instead.
 */

#pragma once
#include "device/DeviceCommands.h"
#include "device/DeviceInfo.h"

// scale state
#define STATE_VERSION    7 // update whenver structure changes
#define STATE_ADDRESS    0 // EEPROM storage location

struct DeviceState {
  const int version = STATE_VERSION;
  bool locked = false; // whether state is locked
  bool state_logging = false; // whether state is logged (whenever there is a change)
  bool data_logging = false; // whether data is logged

  DeviceState() {};
  DeviceState(bool locked, bool state_logging, bool data_logging) :
    locked(locked), state_logging(state_logging), data_logging(data_logging) {}
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
