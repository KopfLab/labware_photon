#pragma once

#include "LoggerComponent.h"

/* commands */
#define CMD_SETTING           "setting" // Logger "setting yay/nay [notes]" : turns setting on/off
  #define CMD_SETTING_ON      "yay"
  #define CMD_SETTING_OFF     "nay"

// test text
static void getStateSettingText(bool setting, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_SETTING, setting, CMD_SETTING_ON, CMD_SETTING_OFF, target, size, pattern, include_key);
}
static void getStateSettingText(bool setting, char* target, int size, bool value_only = false) {
  if (value_only) getStateSettingText(setting, target, size, PATTERN_V_SIMPLE, false);
  else getStateSettingText(setting, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// Example state
struct ExampleState {
  bool setting = false;
  uint8_t version = 3;
  ExampleState() {};
  ExampleState(bool setting) : setting(setting) {}
};

// Example component
class ExampleLoggerComponent : public LoggerComponent
{

  private:

    ExampleState *state;

  public:
    
    // constructor
    ExampleLoggerComponent (const char *id, LoggerController *ctrl, ExampleState *state) : LoggerComponent(id, ctrl, false), state(state) {}

    /* state persistance */
    virtual size_t getStateSize() { return(sizeof(*state)); }

    virtual void saveState() { 
      EEPROM.put(eeprom_start, *state);
      #ifdef STATE_DEBUG_ON
        Serial.printf("INFO: component '%s' state saved in memory (if any updates were necessary)\n", id);
      #endif
    }; 

    virtual bool restoreState() {
      ExampleState *saved_state = new ExampleState();
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

    /* init */
    virtual void init() {
        LoggerComponent::init();
        Serial.println("example component init");
    };

    /* commands */
    virtual bool parseCommand(LoggerCommand *command) {
        return(parseSetting(command));
    };

    // parse setting
    bool parseSetting(LoggerCommand *command) {
      // decision tree
      if (command->parseVariable(CMD_SETTING)) {
        command->extractValue();
        if (command->parseValue(CMD_SETTING_ON)) {
          command->success(changeStateSetting(true));
        } else if (command->parseValue(CMD_SETTING_OFF)) {
          command->success(changeStateSetting(false));
        }
        getStateSettingText(state->setting, command->data, sizeof(command->data));
      }
      return(command->isTypeDefined());
    }

    // change setting
    bool changeStateSetting (bool on) {
      bool changed = on != state->setting;

      if (changed) state->setting = on;
      
      #ifdef STATE_DEBUG_ON
        if (changed)
          on ? Serial.println("INFO: setting turned on") : Serial.println("INFO: setting turned off");
        else
          on ? Serial.println("INFO: setting already on") : Serial.println("INFO: setting already off");
      #endif

      if (changed) saveState();

      return(changed);
    }

    /* state display */
    virtual void updateDisplayStateInformation() {
      getStateSettingText(state->setting, lcd_buffer, sizeof(lcd_buffer), true);
      ctrl->lcd->printLine(2, lcd_buffer);
    };


    /* state variable */
    virtual void assembleStateVariable() {
      char pair[60];
      getStateSettingText(state->setting, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
    };

};