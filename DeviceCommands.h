// NOTE: consider extracting the log part from the command and keep as a separate class
// would make it easier to use for logging start-up and other extra-ordinatory events
#pragma once
#include "device/DeviceInfo.h"

// return codes:
//  -  0 : success without warning
//  - >0 : success with warnings
//  - <0 : failed with errors
#define CMD_RET_UNDEFINED           -100 // undefined behavior
#define CMD_RET_SUCCESS                0 // succes = 0
#define CMD_RET_ERR                   -1 // errors < 0
#define CMD_RET_ERR_TEXT              "undefined error"
#define CMD_RET_ERR_LOCKED            -2 // error locked
#define CMD_RET_ERR_LOCKED_TEXT       "locked"
#define CMD_RET_ERR_CMD               -3 // invalid command
#define CMD_RET_ERR_CMD_TEXT          "invalid command"
#define CMD_RET_ERR_VAL               -4 // invalid value
#define CMD_RET_ERR_VAL_TEXT          "invalid value"
#define CMD_RET_WARN_NO_CHANGE         1 // state unchaged because it was already the same
#define CMD_RET_WARN_NO_CHANGE_TEXT    "state already as requested"

// command log types
#define CMD_LOG_TYPE_UNDEFINED        "undefined"
#define CMD_LOG_TYPE_ERROR            "error"
#define CMD_LOG_TYPE_STATE_CHANGED    "state changed"
#define CMD_LOG_TYPE_STATE_UNCHANGED  "state unchanged"

// control
#define CMD_LOCK            "lock" // device "lock on/off [notes]" : locks/unlocks the device
  #define CMD_LOCK_ON         "on"
  #define CMD_LOCK_OFF        "off"

// logging
#define CMD_STATE_LOG       "state-log" // device "state-log on/off [notes]" : turns state logging on/off
  #define CMD_STATE_LOG_ON     "on"
  #define CMD_STATE_LOG_OFF    "off"
#define CMD_DATA_LOG       "data-log" // device "data-log on/off [notes]" : turns data logging on/off
  #define CMD_DATA_LOG_ON     "on"
  #define CMD_DATA_LOG_OFF    "off"

struct DeviceCommand {

    // command message
    char command[CMD_MAX_CHAR];
    char buffer[CMD_MAX_CHAR];
    char variable[25];
    char value[20];
    char units[20];
    char notes[CMD_MAX_CHAR];

    // command outcome
    char type[20]; // command type
    char msg[50]; // log message
    char data[50]; // data text
    int ret_val; // return value

    // constructors
    DeviceCommand() {};

    // command extraction
    void reset();
    void load(String& command_string);
    void extractParam(char* param, int size);
    void extractVariable();
    void extractValue();
    void extractUnits();
    void assignNotes();

    // command parsing
    bool parseVariable(char* cmd);
    bool parseValue(char* cmd);

    // command status
    bool isTypeDefined(); // whether the command type was found
    void success(bool state_changed);
    void success(bool state_changed, bool capture_notes);
    void warning(int code, char* text);
    void error(int code, char* text);
    void error();
    void errorLocked();
    void errorCommand();
    void errorValue();

    // set a log message
    void setLogMsg(char* log_msg);

    // other speciality commands
    void makeStartupLog();
};

void DeviceCommand::makeStartupLog() {
  reset();
  strcpy(type, "startup");
  getInfoKeyValue(data, sizeof(data), "startup", "complete", PATTERN_KV_JSON_QUOTED);
  ret_val = CMD_RET_SUCCESS;
}

/****** COMMAND PARSING *******/

void DeviceCommand::load(String& command_string) {
  reset();
  command_string.toCharArray(command, sizeof(command));
  strcpy(buffer, command);
}

void DeviceCommand::reset() {
  buffer[0] = 0;
  command[0] = 0;
  value[0] = 0;
  units[0] = 0;
  notes[0] = 0;

  strcpy(type, CMD_LOG_TYPE_UNDEFINED);
  msg[0] = 0;
  data[0] = 0;
  ret_val = CMD_RET_UNDEFINED;
}

// capture command excerpt (until next space) in param
// using char array pointers instead of String to make sure we don't get memory leaks here
// providing size to be sure to be on the safe side
void DeviceCommand::extractParam(char* param, int size) {
  int space = strcspn(buffer, " ");
  // size safety check
  if (space < size) {
    strncpy (param, buffer, space);
    param[space] = 0;
  } else {
    strncpy (param, buffer, size);
    param[size] = 0;
  }
  // clean up buffer
  if (space == strlen(buffer)) {
    buffer[0] = 0;
  } else {
    for(int i = space+1; i <= strlen(buffer); i+=1) {
      buffer[i-space-1] = buffer[i];
    }
  }
}

// assigns the next extractable parameter to variable
void DeviceCommand::extractVariable() {
  extractParam(variable, sizeof(variable));
}

// assigns the next extractable paramter to value
void DeviceCommand::extractValue() {
  extractParam(value, sizeof(value));
}

// assigns the next extractable parameter to units
void DeviceCommand::extractUnits() {
  extractParam(units, sizeof(units));
}

// takes the remainder of the command buffer and assigns it to the message
void DeviceCommand::assignNotes() {
  strncpy(notes, buffer, sizeof(notes));
}

// check if variable has the specific value
bool DeviceCommand::parseVariable(char* cmd) {
  if (strcmp(variable, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

// check if variable has the specific value
bool DeviceCommand::parseValue(char* cmd) {
  if (strcmp(value, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

/****** COMMAND STATUS *******/

bool DeviceCommand::isTypeDefined() {
  return(ret_val != CMD_RET_UNDEFINED);
}

void DeviceCommand::success(bool state_changed) { success(state_changed, true); }
void DeviceCommand::success(bool state_changed, bool capture_notes) {
  if (state_changed) {
    ret_val = CMD_RET_SUCCESS;
    strcpy(type, CMD_LOG_TYPE_STATE_CHANGED);
  } else {
    warning(CMD_RET_WARN_NO_CHANGE, CMD_RET_WARN_NO_CHANGE_TEXT);
    strcpy(type, CMD_LOG_TYPE_STATE_UNCHANGED);
  }
  if (capture_notes) {
    assignNotes();
  }
}

void DeviceCommand::warning(int code, char* text) {
  // warning affects return code and adds warning message
  ret_val = code;
  setLogMsg(text);
}

void DeviceCommand::error(int code, char* text) {
  // error changes type and stores entire command in notes
  ret_val = code;
  setLogMsg(text);
  strncpy(type, CMD_LOG_TYPE_ERROR, sizeof(type) - 1);
  strcpy(notes, command); // store entire command in notes
}

void DeviceCommand::error() {
  error(CMD_RET_ERR, CMD_RET_ERR_TEXT);
}

void DeviceCommand::errorLocked() {
  error(CMD_RET_ERR_LOCKED, CMD_RET_ERR_LOCKED_TEXT);
}

void DeviceCommand::errorCommand() {
  error(CMD_RET_ERR_CMD, CMD_RET_ERR_CMD_TEXT);
}

void DeviceCommand::errorValue() {
  error(CMD_RET_ERR_VAL, CMD_RET_ERR_VAL_TEXT);
}

void DeviceCommand::setLogMsg(char* log_msg) {
  strncpy(msg, log_msg, sizeof(msg) - 1);
  msg[sizeof(msg)-1] = 0;
}
