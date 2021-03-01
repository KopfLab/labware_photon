#pragma once
#include "SerialReaderLoggerComponent.h"

/*** general commands ***/

#define CMD_MFC_ID          "mfc"     // set serial ID
#define CMD_MFC_FLOW        "flow"    // set flow rate

/*** general state ***/
struct MFCState {

    char mfc_id[10];
    uint8_t version = 1;

    MFCState() {};

    MFCState (const char* mfc_id) {
          strncpy(this->mfc_id, mfc_id, sizeof(this->mfc_id) - 1); this->mfc_id[sizeof(this->mfc_id) - 1] = 0;
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

/*** component ***/
class MFCLoggerComponent : public SerialReaderLoggerComponent
{

  private:


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

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parseMFCID(LoggerCommand *command);
    
    /*** state changes ***/
    virtual bool changeMFCID(char* mfc_id);
    

    /*** manage data ***/
    // implement in derived classes

    /*** logger state variable ***/
    virtual void assembleStateVariable();

};