#include "application.h"
#include "MFCLoggerComponent.h"

/*** state management ***/

size_t MFCLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void MFCLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool MFCLoggerComponent::restoreState() {
    MFCState *saved_state = new MFCState();
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

void MFCLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}

/*** command parsing ***/

bool MFCLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseMFCID(command)) {
    // calc rate command parsed
  }
  return(command->isTypeDefined());
}

bool MFCLoggerComponent::parseMFCID(LoggerCommand *command) {
  if (command->parseVariable(CMD_MFC_ID)) {
    command->extractValue();
    command->success(changeMFCID(command->value));
    getStateMFCIDText(state->mfc_id, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

bool MFCLoggerComponent::changeMFCID (char* mfc_id) {
  bool changed = strcmp(mfc_id, state->mfc_id) != 0;

  if (changed) strncpy(state->mfc_id, mfc_id, sizeof(state->mfc_id) - 1); state->mfc_id[sizeof(state->mfc_id) - 1] = 0;

  if (changed) {
    Serial.printlnf("INFO: changing mfc ID to %s", state->mfc_id);
    clearData(true);
  } else {
    Serial.printlnf("INFO: mfc ID is already %s", state->mfc_id);
  }
  
  if (changed) saveState();

  return(changed);
}

/*** logger state variable ***/

void MFCLoggerComponent::assembleStateVariable() {
    char pair[60];
    getStateMFCIDText(state->mfc_id, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}