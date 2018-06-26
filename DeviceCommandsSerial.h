#pragma once
#include "device/DeviceCommands.h"

// time units
#define CMD_TIME_MS     "ms" // milli seconds
#define CMD_TIME_SEC    "s"  // seconds
#define CMD_TIME_MIN    "m"  // minutes
#define CMD_TIME_HR     "h"  // hours

// read rate
#define CMD_DATA_READ_PERIOD          "read-period" // device read-period number unit [notes] : timing between each data read, may not be smaller than device defined minimum and may not be smaller than log period (if a time)
  #define CMD_DATA_READ_PERIOD_MS     CMD_TIME_MS   // device read-period 200ms : read every 200 milli seconds
  #define CMD_DATA_READ_PERIOD_SEC    CMD_TIME_SEC  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MIN    CMD_TIME_MIN  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MANUAL "manual"      // read only upon manual trigger from the device (may not be available on all devices), typically most useful with 'log-period 1x'

// logging rate
#define CMD_DATA_LOG_PERIOD            "log-period" // device log-period number unit [notes] : timing between each data logging (if log is on)
  #define CMD_DATA_LOG_PERIOD_NUMBER   "x"          // device log-period 5x : every 5 data recordings
  #define CMD_DATA_LOG_PERIOD_SEC      CMD_TIME_SEC // device log-period 20s : every 20 seconds
  #define CMD_DATA_LOG_PERIOD_MIN      CMD_TIME_MIN // device log-period 3m : every 3 minutes
  #define CMD_DATA_LOG_PERIOD_HR       CMD_TIME_HR  // device log-period 1h : every hour

// errors
#define CMD_RET_ERR_LOG_SMALLER_READ         -11 // log period cannot be smaller than read period!
#define CMD_RET_ERR_LOG_SMALLER_READ_TEXT    "log period must be larger than read period"
#define CMD_RET_ERR_READ_LARGER_MIN          -12 // read period cannot be smaller than its minimum
#define CMD_RET_ERR_READ_LARGER_MIN_TEXT     "read period must be larger than minimum"
