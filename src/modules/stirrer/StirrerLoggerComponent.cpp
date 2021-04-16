#include "application.h"
#include "StirrerLoggerComponent.h"

/*** loop ***/

void StirrerLoggerComponent::update() {
    SerialReaderLoggerComponent::update();
    if (update_stirrer && data_read_status == DATA_READ_IDLE && isPastRequestDelay()) {
        updateStirrer();
        update_stirrer = false;
    }
}

/*** setup ***/

uint8_t StirrerLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // idx, key, units, digits
    data.push_back(LoggerData(1, "speed", "rpm", 1));
    data.push_back(LoggerData(1, "speed", "rpm", 1));
    return(start_idx + data.size());
}

void StirrerLoggerComponent::completeStartup() {
    SerialReaderLoggerComponent::completeStartup();
    Serial.printlnf("INFO: logging %s speed at startup (%.3f rpm)", id, state->rpm);
    logCurrent(state->status, state->rpm);
}

/*** state management ***/
    
size_t StirrerLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void StirrerLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool StirrerLoggerComponent::restoreState() {
    StirrerState *saved_state = new StirrerState();
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

void StirrerLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}

/*** command parsing ***/

bool StirrerLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseStatus(command)) {
    // stirrer state command parsed
  } else if (parseSpeed(command)) {
    // stirrer speed command parsed
  }
  return(command->isTypeDefined());
}

bool StirrerLoggerComponent::parseStatus(LoggerCommand *command) {
    if (command->parseVariable(CMD_STIRRER_START)) {
    // start
    command->success(changeStatus(STIRRER_STATUS_ON));
  } else if (command->parseVariable(CMD_STIRRER_STOP)) {
    // stop
    command->success(changeStatus(STIRRER_STATUS_OFF));
  } 

  // set command data if type defined
  if (command->isTypeDefined()) {
    getStirrerStateStatusInfo(state->status, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool StirrerLoggerComponent::parseSpeed(LoggerCommand *command) {
  if (command->parseVariable(CMD_STIRRER_SPEED)) {
    // FIXME: process manual as well and figure out how to switch from m
    command->extractValue();
    command->extractUnits();
    // check if units match
    if (command->parseUnits(STIRRER_SPEED_RPM)) {
      // units are correct -> save value
      char* end;
      float number = strtof (command->value, &end);
      int converted = end - command->value;
      if (converted > 0) {
        // valid number
        if (state->rpm > 0) changeStatus(STIRRER_STATUS_ON);
        else changeStatus(STIRRER_STATUS_OFF);
        command->success(changeSpeed(number, true));
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
    getStirrerStateSpeedInfo(state->rpm, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

/*** state changes ***/

bool StirrerLoggerComponent::changeStatus(int status) {

  // only update if necessary
  bool changed = status != state->status;

  if (changed && status == STIRRER_STATUS_ON) {
    Serial.printf("INFO: %s status updating to ON\n", id);
  } else if (changed && status == STIRRER_STATUS_OFF) {
    Serial.printf("INFO: %s status updating to OFF\n", id);
  } else if (changed && status == STIRRER_STATUS_MANUAL) {
    Serial.printf("INFO: %s status updating to MANUAL\n", id);
  } else if (!changed && status == STIRRER_STATUS_ON) {
    Serial.printf("INFO: %s status unchanged (ON)\n", id);  
  } else if (!changed && status == STIRRER_STATUS_OFF) {
    Serial.printf("INFO: %s status unchanged (OFF)\n", id);  
  } else if (!changed && status == STIRRER_STATUS_MANUAL) {
    Serial.printf("INFO: %s status unchanged (MANUAL)\n", id);  
  }

  if (changed) {
    logChange(state->status, state->rpm, status, state->rpm);
    state->status = status;
    saveState();
    update_stirrer = true;
  }
  return(changed);
}

bool StirrerLoggerComponent::changeSpeed(float rpm, bool update) {
    bool changed = fabs(state->rpm - rpm) > 0.0001;

    if (changed) {
        Serial.printlnf("INFO: changing %s speed from %.3f to %.3f rpm", id, state->rpm, rpm);
        logChange(state->status, state->rpm, state->status, rpm);
        state->rpm = rpm;
        saveState();
        // update stirrer if it's actually on
        if (update && state->status == STIRRER_STATUS_ON) update_stirrer = true;
    } else {
        Serial.printf("INFO: %s speed staying unchanged (%.3f rpm)\n", id, state->rpm);
    }
    return(changed);
}

void StirrerLoggerComponent::logChange(int status_current, float rpm_current, int status_change, float rpm_change) {
    // OFF status as 0.0 rpm
    if (status_current == STIRRER_STATUS_OFF) rpm_current = 0.0;
    if (status_change == STIRRER_STATUS_OFF) rpm_change = 0.0;

    // recording change
    data[1].setNewestDataTime(millis() - 1);
    data[1].setNewestValue(rpm_current);
    data[1].saveNewestValue(false);
    data[0].setNewestDataTime(millis());
    data[0].setNewestValue(rpm_change);
    data[0].saveNewestValue(false);
    
    // log and clear the step change
    logData();
    data[1].clear();
}

void StirrerLoggerComponent::logCurrent(int status_current, float rpm_current) {
    // OFF status as 0.0 rpm
    if (status_current == STIRRER_STATUS_OFF) rpm_current = 0.0;
    
    // recording current
    data[0].setNewestDataTime(millis());
    data[0].setNewestValue(rpm_current);
    data[0].saveNewestValue(false);

    // log and clear
    logData();
}

/*** manage data ***/

void StirrerLoggerComponent::finishData() {
    // rpm
    if (error_counter == 0) {
        data[0].setNewestValue(value_buffer);
        data[0].saveNewestValue(false); // don't average
        // see if anything changed
        if (fabs(state->rpm - data[0].getValue()) > 0.0001) {
            if (state->status == STIRRER_STATUS_MANUAL) {
                // update speed
                changeSpeed(data[0].getValue(), false);
            } else {
                // reset back to what is set (not in manual mode)
                changeSpeed(state->rpm, true);
            }
        }
    }
}

/*** stirrer functions ***/

void StirrerLoggerComponent::updateStirrer() {
    // implement in derived classes to communicate state->status and state->rpm to the stirrer
}
