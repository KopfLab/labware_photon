#include "application.h"
#include "MFCLoggerComponent.h"

/*** loop ***/
void MFCLoggerComponent::update() {
    SerialReaderLoggerComponent::update();
    if (update_mfc && data_read_status == DATA_READ_IDLE && isPastRequestDelay()) {
        updateMFC();
        update_mfc = false;
    }
}


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
    // MFC ID command parsed
  } else if (parseStatus(command)) {
    // MFC state command parsed
  } else if (parseSetpoint(command)) {
    // MFC flow command parsed
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

bool MFCLoggerComponent::parseStatus(LoggerCommand *command) {
    if (command->parseVariable(CMD_MFC_START)) {
    // start
    command->success(start());
  } else if (command->parseVariable(CMD_MFC_STOP)) {
    // stop
    command->success(stop());
  } 

  // set command data if type defined
  if (command->isTypeDefined()) {
    getMFCStateStatusInfo(state->status, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool MFCLoggerComponent::parseSetpoint(LoggerCommand *command) {
  if (command->parseVariable(CMD_MFC_SETPOINT)) {
    command->extractValue();
    command->extractUnits();
    // check if units match
    if (command->parseUnits(state->units)) {
      // flow in correct units
      char* end;
      float number = strtof (command->value, &end);
      int converted = end - command->value;
      if (converted > 0) {
        // valid number
        command->success(changeSetpoint(number));
      } else {
        // no number, invalid value
        command->errorValue();
      }
    } else {
      command->errorUnits();
    }
  }

  // set command data if type defined
  if (command->isTypeDefined()) {
    getMFCStateSetpointInfo(state->setpoint, state->units, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

/*** state changes ***/

bool MFCLoggerComponent::changeMFCID (char* mfc_id) {
  bool changed = strcmp(mfc_id, state->mfc_id) != 0;

  if (changed) strncpy(state->mfc_id, mfc_id, sizeof(state->mfc_id) - 1); state->mfc_id[sizeof(state->mfc_id) - 1] = 0;

  if (changed) {
    Serial.printlnf("INFO: changing mfc ID to %s", state->mfc_id);
    clearData();
  } else {
    Serial.printlnf("INFO: mfc ID is already %s", state->mfc_id);
  }
  
  if (changed) saveState();

  return(changed);
}

bool MFCLoggerComponent::changeStatus(int status) {

  // only update if necessary
  bool changed = status != state->status;

  if (changed && status == MFC_STATUS_ON) {
      Serial.printf("INFO: %s status updating to ON\n", id);
  } else if (changed && status == MFC_STATUS_OFF) {
      Serial.printf("INFO: %s status updating to OFF\n", id);
  } else if (!changed && status == MFC_STATUS_ON) {
    Serial.printf("INFO: %s status unchanged (ON)\n", id);  
  } else if (!changed && status == MFC_STATUS_OFF) {
    Serial.printf("INFO: %s status unchanged (OFF)\n", id);  
  }

  if (changed) {
    state->status = status;
    saveState();
    logData();
    clearData();
    update_mfc = true;
  }
  return(changed);
}

bool MFCLoggerComponent::start() { return(changeStatus(MFC_STATUS_ON)); }
bool MFCLoggerComponent::stop() { return(changeStatus(MFC_STATUS_OFF)); }

bool MFCLoggerComponent::changeSetpoint(float flow) {
    bool changed = fabs(state->setpoint - flow) > 0.0001;

    if (changed) {
        state->setpoint = flow;
        Serial.printf("INFO: changing %s flow to %.3f %s\n", id, state->setpoint, state->units);
        saveState();
        logData();
        clearData();
        // update MFC if it's actually on
        if (state->status == MFC_STATUS_ON) update_mfc = true;
    } else {
        Serial.printf("INFO: %s flow staying unchanged (%.3f %s)\n", id, state->setpoint, state->units);
    }
    return(changed);
}

/*** MFC functions ***/

void MFCLoggerComponent::updateMFC() {
    // implement in derived classes to communicate state->status and state->setpoint to the MFC
}

/*** logger state variable ***/

void MFCLoggerComponent::assembleStateVariable() {
    char pair[60];
    getStateMFCIDText(state->mfc_id, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
    getMFCStateStatusInfo(state->status, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
    getMFCStateSetpointInfo(state->setpoint, state->units, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}