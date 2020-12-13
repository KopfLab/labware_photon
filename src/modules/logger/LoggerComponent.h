#pragma once

// declare controller to include as member
class LoggerController;

// component class
class LoggerComponent
{

  private:

    // controller
    LoggerController *ctrl;
 
    // state
    size_t eeprom_start;

  public:

    // component name
    const char *name;

    // constructor
    LoggerComponent (const char *name, LoggerController *ctrl) : name(name), ctrl(ctrl) {}

    // state management
    void setEEPROMStart(size_t start) { eeprom_start = start; };
    virtual size_t getStateSize() { return(0); }
    virtual void loadState(bool reset) {
      if (getStateSize() > 0) {
        if (!reset){
            Serial.printf("INFO: trying to restore state from memory for component '%s'\n", name);
            restoreState();
        } else {
            Serial.printf("INFO: resetting state for component '%s' back to default values\n", name);
            saveState();
        }
      }
    };
    virtual bool restoreState(){ };
    virtual void saveState(){ };

    // initialization
    virtual void init() {
       Serial.println("component init");
    };

    // update
    virtual void update() {

    };

};


// derived state
struct DerivedState {
  bool test = false;

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
    DerivedLoggerComponent (const char *name, LoggerController *ctrl, DerivedState *state) : LoggerComponent(name, ctrl), state(state) {}

    virtual size_t getStateSize() { return(sizeof(*state)); }

    virtual void init() {
        Serial.println("derived component init");
    };

};