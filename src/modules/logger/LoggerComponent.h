#pragma once

#include "LoggerCommands.h"

// declare controller to include as member
class LoggerController;

// component class
class LoggerComponent
{

  protected:

     // controller
    LoggerController *ctrl;
 
    // state
    size_t eeprom_start;

  public:

    // component id
    const char *id;

    // constructor
    LoggerComponent (const char *id, LoggerController *ctrl) : id(id), ctrl(ctrl) {}

    // state management
    void setEEPROMStart(size_t start) { eeprom_start = start; };
    virtual size_t getStateSize() { return(0); }
    virtual void loadState(bool reset) {
      if (getStateSize() > 0) {
        if (!reset){
            Serial.printf("INFO: trying to restore state from memory for component '%s'\n", id);
            restoreState();
        } else {
            Serial.printf("INFO: resetting state for component '%s' back to default values\n", id);
            saveState();
        }
      }
    };
    virtual bool restoreState(){ return(false); };
    virtual void saveState(){ };

    // initialization
    virtual void init() {
       Serial.printf("INFO: initializing component '%s'...\n", id);
    };

    // update
    virtual void update() {

    };

    // commands
    virtual bool parseCommand(LoggerCommand *command) {
      return(false);
    };

    // state variable
    virtual void assembleLoggerStateVariable() {
    };

};
