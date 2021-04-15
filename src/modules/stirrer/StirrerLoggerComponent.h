// serial overhead stirrer
#pragma once
#include "SerialReaderLoggerComponent.h"

/*** commands ***/

// status
#define CMD_STIRRER_START       "start" // device start [msg] : starts the stepper
#define CMD_STIRRER_STOP        "stop" // device stop [msg] : stops the stepper (stops power)

// speed
#define CMD_STIRRER_SPEED       "speed" // device speed number rpm/fpm [msg] : set the stepper speed
#define STIRRER_SPEED_RPM       "rpm" // device speed number rpm [msg] : set speed in rotations per minute (requires step-angle)

/*** state ***/

#define STIRRER_STATUS_ON        1
#define STIRRER_STATUS_OFF       2
#define STIRRER_STATUS_MANUAL    3

/* state */
struct StirrerState {

  unsigned int status = STIRRER_STATUS_MANUAL; //  on/off/manual
  float rpm = 0; // setpoint speed in rotations / minute

  uint8_t version = 1;

  StirrerState() {};

  StirrerState (float rpm, unsigned int status) : rpm(rpm), status(status) {};

};

/*** state variable formatting ***/

// status info
static void getStirrerStateStatusInfo(unsigned int status, char* target, int size, char* pattern, bool include_key = true)  {
  if (status == STIRRER_STATUS_ON)
    getStateStringText("status", "on", target, size, pattern, include_key);
  else if (status == STIRRER_STATUS_OFF)
    getStateStringText("status", "off", target, size, pattern, include_key);
  else if (status == STIRRER_STATUS_MANUAL)
    getStateStringText("status", "manual", target, size, pattern, include_key);
  else // should never happen
    getStateStringText("status", "?", target, size, pattern, include_key);
}

// speed
static void getStirrerStateSpeedInfo(float rpm, char* target, int size, char* pattern, bool include_key = true) {
  int decimals = find_signif_decimals(rpm, 3, true, 3); // 3 significant digits by default, max 3 after decimals
  getStateDoubleText("speed", rpm, "rpm", target, size, pattern, decimals, include_key);
}

/*** component ***/
class StirrerLoggerComponent : public SerialReaderLoggerComponent
{

  protected:

    bool update_stirrer = false; // flag for stirrer update as soon as serial is IDLE

  public:

    // state
    StirrerState* state;

    /*** constructors ***/
    // stirrer has a global time offset
    StirrerLoggerComponent (const char *id, LoggerController *ctrl, StirrerState* state, const long baud_rate, const long serial_config, const char *request_command, unsigned int data_pattern_size) : 
      SerialReaderLoggerComponent(id, ctrl, false, baud_rate, serial_config, request_command, data_pattern_size), state(state) {}
    StirrerLoggerComponent (const char *id, LoggerController *ctrl, StirrerState* state, const long baud_rate, const long serial_config, const char *request_command) : 
      StirrerLoggerComponent(id, ctrl, state, baud_rate, serial_config, request_command, 0) {}
    StirrerLoggerComponent (const char *id, LoggerController *ctrl, StirrerState* state, const long baud_rate, const long serial_config, unsigned int data_pattern_size) : 
      StirrerLoggerComponent(id, ctrl, state, baud_rate, serial_config, "", data_pattern_size) {}
    StirrerLoggerComponent (const char *id, LoggerController *ctrl, StirrerState* state, const long baud_rate, const long serial_config) : 
      StirrerLoggerComponent(id, ctrl, state, baud_rate, serial_config, "", 0) {}

    // FIXME: continue here


};