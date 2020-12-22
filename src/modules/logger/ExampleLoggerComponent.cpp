#include "application.h"
#include "ExampleLoggerComponent.h"

/*** state variable formatting ***/

static void getStateSettingText(bool setting, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_SETTING, setting, CMD_SETTING_ON, CMD_SETTING_OFF, target, size, pattern, include_key);
}

static void getStateSettingText(bool setting, char* target, int size, bool value_only = false) {
  if (value_only) getStateSettingText(setting, target, size, PATTERN_V_SIMPLE, false);
  else getStateSettingText(setting, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/*** setup ***/

// setup data vector - override in derived clases, has to return the new index
uint8_t ExampleLoggerComponent::setupDataVector(uint8_t start_idx) { 
    start_idx = LoggerComponent::setupDataVector(start_idx);
    return(start_idx); 
};

void ExampleLoggerComponent::init() {
    LoggerComponent::init();
    Serial.println("example component init");
}

/*** state management ***/

size_t ExampleLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void ExampleLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    #ifdef STATE_DEBUG_ON
    Serial.printf("INFO: component '%s' state saved in memory (if any updates were necessary)\n", id);
    #endif
} 

bool ExampleLoggerComponent::restoreState() {
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
}

/*** command parsing ***/

bool ExampleLoggerComponent::parseCommand(LoggerCommand *command) {
    return(parseSetting(command));
}

bool ExampleLoggerComponent::parseSetting(LoggerCommand *command) {
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

/*** state changes ***/

bool ExampleLoggerComponent::changeStateSetting (bool on) {
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

/*** state info to LCD display ***/

void ExampleLoggerComponent::updateDisplayStateInformation() {
    getStateSettingText(state->setting, lcd_buffer, sizeof(lcd_buffer), true);
    ctrl->lcd->printLine(2, lcd_buffer);
}

/*** logger state variable ***/

void ExampleLoggerComponent::assembleStateVariable() {
    char pair[60];
    getStateSettingText(state->setting, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}
