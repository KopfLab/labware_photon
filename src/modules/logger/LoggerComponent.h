#pragma once

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

};


// derived state
struct DerivedState {
  bool test = false;
  uint8_t version = 2;
  DerivedState() {};
  DerivedState(bool test) : test(test) {}
};

// derived component
class DerivedLoggerComponent : public LoggerComponent
{

  private:

    DerivedState *state;

  public:

    
    // constructor
    DerivedLoggerComponent (const char *id, LoggerController *ctrl, DerivedState *state) : LoggerComponent(id, ctrl), state(state) {}

    virtual size_t getStateSize() { return(sizeof(*state)); }

    virtual void saveState() { 
      EEPROM.put(eeprom_start, *state);
      #ifdef STATE_DEBUG_ON
        Serial.printf("INFO: component '%s' state saved in memory (if any updates were necessary)\n", id);
      #endif
    }; 
    virtual bool restoreState() {
      DerivedState *saved_state = new DerivedState();
      EEPROM.get(eeprom_start, *saved_state);
      bool recoverable = saved_state->version == state->version;
      if(recoverable) {
        EEPROM.get(eeprom_start, *state);
        Serial.printf("INFO: successfully restored component state from memory (state version %d)\n", state->version);
      } else {
        Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
        saveState();
      }
      return(recoverable);
    };

    virtual void init() {
        LoggerComponent::init();
        Serial.println("derived component init");
    };

};