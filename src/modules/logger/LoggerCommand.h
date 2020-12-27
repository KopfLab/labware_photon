#pragma once
#include "application.h"
#include "LoggerConstants.h"

// return codes:
//  -  0 : success without warning
//  - >0 : success with warnings
//  - <0 : failed with errors
#define CMD_RET_UNDEFINED                   -100 // undefined behavior
#define CMD_RET_SUCCESS                     0 // succes = 0
#define CMD_RET_ERR                         -1 // errors < 0
#define CMD_RET_ERR_TEXT                    "undefined error"
#define CMD_RET_ERR_LOCKED                  -2 // error locked
#define CMD_RET_ERR_LOCKED_TEXT             "locked"
#define CMD_RET_ERR_CMD                     -3 // invalid command
#define CMD_RET_ERR_CMD_TEXT                "invalid command"
#define CMD_RET_ERR_VAL                     -4 // invalid value
#define CMD_RET_ERR_VAL_TEXT                "invalid value"
#define CMD_RET_ERR_UNITS                   -5 // invalid units
#define CMD_RET_ERR_UNITS_TEXT              "invalid units"
#define CMD_RET_ERR_NOT_A_READER            -10 // not a data reader
#define CMD_RET_ERR_NOT_A_READER_TEXT       "logger is not a data reader"
#define CMD_RET_ERR_LOG_SMALLER_READ        -11 // log period cannot be smaller than read period!
#define CMD_RET_ERR_LOG_SMALLER_READ_TEXT   "log period must be larger than read period"
#define CMD_RET_ERR_READ_LARGER_MIN         -12 // read period cannot be smaller than its minimum
#define CMD_RET_ERR_READ_LARGER_MIN_TEXT    "read period must be larger than minimum"
#define CMD_RET_WARN_NO_CHANGE              1 // state unchaged because it was already the same
#define CMD_RET_WARN_NO_CHANGE_TEXT         "state already as requested"

// command log types
#define CMD_LOG_TYPE_UNDEFINED              "undefined"
#define CMD_LOG_TYPE_UNDEFINED_SHORT        "UDEF"
#define CMD_LOG_TYPE_ERROR                  "error"
#define CMD_LOG_TYPE_ERROR_SHORT            "ERR"
#define CMD_LOG_TYPE_STATE_CHANGED          "state changed"
#define CMD_LOG_TYPE_STATE_CHANGED_SHORT    "OK"
#define CMD_LOG_TYPE_STATE_UNCHANGED        "state unchanged"
#define CMD_LOG_TYPE_STATE_UNCHANGED_SHORT  "SAME"

// locking
#define CMD_LOCK            "lock" // device "lock on/off [notes]" : locks/unlocks the Logger
  #define CMD_LOCK_ON         "on"
  #define CMD_LOCK_OFF        "off"

// logging
#define CMD_STATE_LOG       "state-log" // device "state-log on/off [notes]" : turns state logging on/off
  #define CMD_STATE_LOG_ON     "on"
  #define CMD_STATE_LOG_OFF    "off"
#define CMD_DATA_LOG       "data-log" // device "data-log on/off [notes]" : turns data logging on/off
  #define CMD_DATA_LOG_ON     "on"
  #define CMD_DATA_LOG_OFF    "off"

// time units
#define CMD_TIME_MS     "ms" // milli seconds
#define CMD_TIME_SEC    "s"  // seconds
#define CMD_TIME_MIN    "m"  // minutes
#define CMD_TIME_HR     "h"  // hours
#define CMD_TIME_DAY    "d"  // days

// logging rate
#define CMD_DATA_LOG_PERIOD            "log-period" // device log-period number unit [notes] : timing between each data logging (if log is on)
  #define CMD_DATA_LOG_PERIOD_NUMBER   "x"          // device log-period 5x : every 5 data recordings
  #define CMD_DATA_LOG_PERIOD_SEC      CMD_TIME_SEC // device log-period 20s : every 20 seconds
  #define CMD_DATA_LOG_PERIOD_MIN      CMD_TIME_MIN // device log-period 3m : every 3 minutes
  #define CMD_DATA_LOG_PERIOD_HR       CMD_TIME_HR  // device log-period 1h : every hour

// reading rate
#define CMD_DATA_READ_PERIOD          "read-period" // device read-period number unit [notes] : timing between each data read, may not be smaller than device defined minimum and may not be smaller than log period (if a time)
  #define CMD_DATA_READ_PERIOD_MS     CMD_TIME_MS   // device read-period 200ms : read every 200 milli seconds
  #define CMD_DATA_READ_PERIOD_SEC    CMD_TIME_SEC  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MIN    CMD_TIME_MIN  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MANUAL "manual"      // read only upon manual trigger from the device (may not be available on all devices), typically most useful with 'log-period 1x'

// reset
#define CMD_RESET      "reset" 
  #define CMD_RESET_DATA  "data" // device "reset data" : reset the data stored in the device
  #define CMD_RESET_STATE "state" // device "reset state" : resets state (causes a device restart)
// restart

#define CMD_RESTART    "restart" // device "restart" : restarts the device

struct LoggerCommand {

    // command message
    char command[CMD_MAX_CHAR];
    char buffer[CMD_MAX_CHAR];
    char variable[25];
    char value[20];
    char units[20];
    char notes[CMD_MAX_CHAR];

    // command outcome
    char type[20]; // command type
    char type_short[10]; // short version of the command type (for lcd)
    char msg[50]; // log message
    char data[50]; // data text
    int ret_val; // return value

    // constructors
    LoggerCommand() {};

    // command extraction
    void reset();
    void load(String& command_string);
    void extractParam(char* param, uint size);
    void extractVariable();
    void extractValue();
    void extractUnits();
    void assignNotes();

    // command parsing
    bool parseVariable(char* cmd);
    bool parseValue(char* cmd);
    bool parseUnits(char* cmd);

    // command status
    bool isTypeDefined(); // whether the command type was found
    bool hasStateChanged(); // whether state has been changed successfully by the command
    void success(bool state_changed);
    void success(bool state_changed, bool capture_notes);
    void warning(int code, const char* text);
    void error(int code, const char* text);
    void error();
    void errorLocked();
    void errorCommand();
    void errorValue();
    void errorUnits();

    // set a log message
    void setLogMsg(const char* log_msg);

};

