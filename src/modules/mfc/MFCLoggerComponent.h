#pragma once
#include "SerialReaderLoggerComponent.h"

/*** general commands ***/

#define CMD_MFC_ID          "mfc"       // device mfc id [msg] : set serial ID for the MFC
#define CMD_MFC_SETPOINT    "setpoint"  // device flow numbers units [msg] : set flow rate, must be in the currently registered units
#define CMD_MFC_START       "start"     // device start [msg] : starts the MFC flow
#define CMD_MFC_STOP        "stop"      // device stop [msg] : stops the MFC flow

/*** general state ***/

#define MFC_STATUS_ON        1
#define MFC_STATUS_OFF       2

struct MFCState {

    char mfc_id[10];
    unsigned int status; // MFC_STATUS_ON, MFC_STATUS_OFF
    float setpoint;      // setpoint mass flow rate [in units]
    char units[20];      // setpoint mass flow units
    uint8_t version = 2;

    MFCState() {};

    MFCState(const char* mfc_id) : MFCState(mfc_id, MFC_STATUS_OFF, 0, "") {};

    MFCState (const char* mfc_id, unsigned int status, float setpoint, const char* units) : status(status), setpoint(setpoint) {
          strncpy(this->mfc_id, mfc_id, sizeof(this->mfc_id) - 1); this->mfc_id[sizeof(this->mfc_id) - 1] = 0;
          strncpy(this->units, units, sizeof(this->units) - 1); this->units[sizeof(this->units) - 1] = 0;
      };

};

/*** state variable formatting ***/

// MFC ID
static void getStateMFCIDText(char* mfc_id, char* target, int size, const char* pattern, bool include_key = true) {
  getStateStringText(CMD_MFC_ID, mfc_id, target, size, pattern, include_key);
}

static void getStateMFCIDText(char* mfc_id, char* target, int size, bool value_only = false) {
  (value_only) ?
    getStateMFCIDText(mfc_id, target, size, PATTERN_V_SIMPLE, false) :
    getStateMFCIDText(mfc_id, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// MFC status info
static void getMFCStateStatusInfo(unsigned int status, char* target, int size, char* pattern, bool include_key = true)  {
  if (status == MFC_STATUS_ON)
    getStateStringText("status", "on", target, size, pattern, include_key);
  else if (status == MFC_STATUS_OFF)
    getStateStringText("status", "off", target, size, pattern, include_key);
  else // should never happen
    getStateStringText("status", "?", target, size, pattern, include_key);
}

static void getMFCStateStatusInfo(unsigned int status, char* target, int size, bool value_only = false) {
  if (value_only) getMFCStateStatusInfo(status, target, size, PATTERN_V_SIMPLE, false);
  else getMFCStateStatusInfo(status, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// MFC setpoint
static void getMFCStateSetpointInfo(float flow, char* units, char* target, int size, char* pattern, bool include_key = true) {
  int decimals = find_signif_decimals(flow, 3, true, 3); // 3 significant digits by default, max 3 after decimals
  getStateDoubleText("setpoint", flow, units, target, size, pattern, decimals, include_key);
}

static void getMFCStateSetpointInfo(float flow, char* units, char* target, int size, bool value_only = false) {
  if (value_only) getMFCStateSetpointInfo(flow, units, target, size, PATTERN_VU_SIMPLE, false);
  else getMFCStateSetpointInfo(flow, units, target, size, PATTERN_KVU_JSON_QUOTED, true);
}

/*** component ***/
class MFCLoggerComponent : public SerialReaderLoggerComponent
{

  protected:

    bool update_mfc = false; // flag for mfc update as soon as serial is IDLE

  public:

    // state
    MFCState* state;

    /*** constructors ***/
    // mfc has a global time offset
    MFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state, const long baud_rate, const long serial_config, const char *request_command, unsigned int data_pattern_size) : 
      SerialReaderLoggerComponent(id, ctrl, false, baud_rate, serial_config, request_command, data_pattern_size), state(state) {}
    MFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state, const long baud_rate, const long serial_config, const char *request_command) : 
      MFCLoggerComponent(id, ctrl, state, baud_rate, serial_config, request_command, 0) {}
    MFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state, const long baud_rate, const long serial_config, unsigned int data_pattern_size) : 
      MFCLoggerComponent(id, ctrl, state, baud_rate, serial_config, "", data_pattern_size) {}
    MFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state, const long baud_rate, const long serial_config) : 
      MFCLoggerComponent(id, ctrl, state, baud_rate, serial_config, "", 0) {}

    /*** loop ***/
    virtual void update();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    virtual bool parseCommand(LoggerCommand *command);
    virtual bool parseMFCID(LoggerCommand *command);
    virtual bool parseStatus(LoggerCommand *command);
    virtual bool parseSetpoint(LoggerCommand *command);

    /*** state changes ***/
    virtual bool changeMFCID(char* mfc_id);
    virtual bool changeStatus(int status);
    virtual bool start(); // start the flow
    virtual bool stop(); // stop the flow
    virtual bool changeSetpoint(float flow);

    /*** MFC functions ***/
    virtual void updateMFC(); // update the actual MFC when status or flow rate changes

    /*** manage data ***/
    // implement in derived classes

    /*** logger state variable ***/
    virtual void assembleStateVariable();

};