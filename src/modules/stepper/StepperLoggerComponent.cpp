#include "application.h"
#include "StepperLoggerComponent.h"

/*** debug ***/
void StepperLoggerComponent::debug() {
    debug_mode = true;
}

/*** setup ***/

uint8_t StepperLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // same index to allow for step transition logging
    // idx, key, units, digits
    data.push_back(LoggerData(1, "speed", "rpm", 1));
    data.push_back(LoggerData(1, "speed", "rpm", 1));
    return(start_idx + data.size()); 
}

void StepperLoggerComponent::init() {
    ControllerLoggerComponent::init();

    // calculate drive limits and initalize stepper
    driver->calculateRpmLimits(board->max_speed, motor->steps, motor->gearing);
    stepper = AccelStepper(AccelStepper::DRIVER, board->step, board->dir);
    stepper.setEnablePin(board->enable);
    stepper.setPinsInverted	(
                driver->dir_cw != LOW,
                driver->step_on != HIGH,
                driver->enable_on != LOW
            );
    stepper.disableOutputs();
    stepper.setMaxSpeed(board->max_speed);

    // microstepping
    state->ms_index = findMicrostepIndexForRpm(state->rpm);
    state->ms_mode = driver->getMode(state->ms_index);
    pinMode(board->ms1, OUTPUT);
    pinMode(board->ms2, OUTPUT);
    pinMode(board->ms3, OUTPUT);
    if (debug_mode) {
        Serial.println("DEBUG: available microstepping modes");
        for (int i = 0; i < driver->ms_modes_n; i++) {
          Serial.printf("   Mode %d: %i steps, max rpm: %.1f\n", i, driver->ms_modes[i].mode, driver->ms_modes[i].rpm_limit);
        }
    }

    updateStepper();
}

void StepperLoggerComponent::completeStartup() {
    ControllerLoggerComponent::completeStartup();
    Serial.printf("INFO: logging %s speed at startup (%.4frpm)\n", id, data[0].getValue());
    logData();
}

/*** loop ***/

void StepperLoggerComponent::update() {
  if (state->status == STATUS_ROTATE) {
    // WARNING: FIXME known bug, when power out, saved rotate status will lead to immediate stop of pump
    if (stepper.distanceToGo() == 0) {
      changeStatus(STATUS_OFF); // disengage if reached target location
      ctrl->updateStateVariable(); // state variable change not connected to a direct commmand
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    stepper.runSpeed();
  }
  ControllerLoggerComponent::update();
}

/*** state management ***/
    
size_t StepperLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void StepperLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool StepperLoggerComponent::restoreState() {
    StepperState *saved_state = new StepperState();
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

void StepperLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}

/*** command parsing ***/

bool StepperLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseStatus(command)) {
    // check for status commands
  } else if (parseDirection(command)) {
    // check for direction commands
  } else if (parseSpeed(command)) {
    // check for speed commands
  //} else if (parseRamp()) {
    // check for ramp commands - TODO
  } else if (parseMS(command)) {
    // check for microstepping commands
  }
  return(command->isTypeDefined());
}

bool StepperLoggerComponent::parseStatus(LoggerCommand *command) {
  if (command->parseVariable(CMD_START)) {
    // start
    command->success(start());
  } else if (command->parseVariable(CMD_STOP)) {
    // stop
    command->success(stop());
  } else if (command->parseVariable(CMD_HOLD)) {
    // hold
    command->success(hold());
  } else if (command->parseVariable(CMD_RUN)) {
    // run - not yet implemented FIXME
  } else if (command->parseVariable(CMD_AUTO)) {
    // auto - not yet implemented FIXME
  } else if (command->parseVariable(CMD_ROTATE)) {
    // rotate
    command->extractValue();
    char* end;
    float number = strtof (command->value, &end);
    int converted = end - command->value;
    if (converted > 0) {
      // valid number
      rotate(number);
      // rotate always counts as new command b/c rotation starts from scratch
      command->success(true);
    } else {
      // no number, invalid value
      command->errorValue();
    }
  }

  // set command data if type defined
  if (command->isTypeDefined()) {
    getStepperStateStatusInfo(state->status, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool StepperLoggerComponent::parseDirection(LoggerCommand *command) {

  if (command->parseVariable(CMD_DIR)) {
    // direction
    command->extractValue();
    if (command->parseValue(CMD_DIR_CW)) {
      // clockwise
      command->success(changeDirection(DIR_CW));
    } else if (command->parseValue(CMD_DIR_CC)) {
      // counter clockwise
      command->success(changeDirection(DIR_CC));
    } else if (command->parseValue(CMD_DIR_SWITCH)) {
      // switch
      command->success(changeDirection(-state->direction));
    } else {
      // invalid
      command->errorValue();
    }
  }

  // set command data if type defined
  if (command->isTypeDefined()) {
    getStepperStateDirectionInfo(state->direction, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool StepperLoggerComponent::parseSpeed(LoggerCommand *command) {

  if (command->parseVariable(CMD_SPEED)) {
    // speed
    command->extractValue();
    command->extractUnits();

    if (command->parseUnits(SPEED_RPM)) {
      // speed rpm
      char* end;
      float number = strtof (command->value, &end);
      int converted = end - command->value;
      if (converted > 0) {
        // valid number
        command->success(changeSpeedRpm(number));
        if( (state->rpm - number) < 0.0 ) {
          // could not set to rpm, hit the max --> set warning
          command->warning(CMD_RET_WARN_MAX_RPM, CMD_RET_WARN_MAX_RPM_TEXT);
        }
      } else {
        // no number, invalid value
        command->errorValue();
      }
    //} else if (command->parseUnits(SPEED_FPM)) {
      // speed fpm
      // TODO
    } else {
      command->errorUnits();
    }
  }

  // set command data if type defined
  if (command->isTypeDefined()) {
    getStepperStateSpeedInfo(state->rpm, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool StepperLoggerComponent::parseMS(LoggerCommand *command) {

  if (command->parseVariable(CMD_STEP)) {
    // microstepping
    command->extractValue();
    if (command->parseValue(CMD_STEP_AUTO)) {
      command->success(changeToAutoMicrosteppingMode());
    } else {
      int ms_mode = atoi(command->value);
      command->success(changeMicrosteppingMode(ms_mode));
    }
  }

  // set command data if type defined
  if (command->isTypeDefined()) {
    getStepperStateMSInfo(state->ms_auto, state->ms_mode, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

/*** state changes ***/

bool StepperLoggerComponent::changeStatus(int status) {

  // only update if necessary
  bool changed = status != state->status;

  (changed) ?
    Serial.printf("INFO: %s status updating to %d\n", id, status):
    Serial.printf("INFO: %s status unchanged (%d)\n", id, status);

  if (changed) {
    state->status = status;
    updateStepper();
    saveState();
  }
  return(changed);
}

bool StepperLoggerComponent::start() { return(changeStatus(STATUS_ON)); }
bool StepperLoggerComponent::stop() { return(changeStatus(STATUS_OFF)); }
bool StepperLoggerComponent::hold() { return(changeStatus(STATUS_HOLD)); }

long StepperLoggerComponent::rotate(float number) {
  long steps = state->direction * number * motor->steps * motor->gearing * state->ms_mode;
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  changeStatus(STATUS_ROTATE);
  return(steps);
}

bool StepperLoggerComponent::changeDirection(int direction) {

  // only update if necessary
  bool changed = (direction == DIR_CW || direction == DIR_CC) && state->direction != direction;

  if (changed)
    (direction == DIR_CW) ? Serial.println("INFO: changing direction to clockwise") : Serial.println("INFO: changing direction to counter clockwise");
  else
    (direction == DIR_CW) ? Serial.println("INFO: direction unchanged (clockwise)") : Serial.println("INFO: direction unchanged (counter clockwise)");

  if (changed) {
    state->direction = direction;
    if (state->status == STATUS_ROTATE) {
      // if rotating to a specific position, changing direction turns the pump off
      Serial.println("INFO: stepper stopped due to change in direction during 'rotate'");
      state->status = STATUS_OFF;
    }
    updateStepper();
    saveState();
  }

  return(changed);
}

bool StepperLoggerComponent::changeSpeedRpm(float rpm) {
  int original_ms_mode = state->ms_mode;
  float original_rpm = state->rpm;
  state->ms_index = findMicrostepIndexForRpm(rpm);
  state->ms_mode = driver->getMode(state->ms_index); // tracked for convenience
  setSpeedWithSteppingLimit(rpm);
  bool changed = state->ms_mode != original_ms_mode | fabs(state->rpm - original_rpm) > 0.0001;

  (changed) ?
    Serial.printf("INFO: changing %s speed to %.3f rpm\n", id, state->rpm) :
    Serial.printf("INFO: %s speed staying unchanged (%.3f rpm)\n", id, state->rpm);

  if (changed) {
    updateStepper();
    saveState();
  }
  return(changed);
}

bool StepperLoggerComponent::changeToAutoMicrosteppingMode() {

  bool changed = !state->ms_auto;

  (changed) ?
    Serial.println("INFO: activating automatic microstepping"):
    Serial.println("INFO: automatic microstepping already active");

  if (changed) {
    state->ms_auto = true;
    state->ms_index = findMicrostepIndexForRpm(state->rpm);
    state->ms_mode = driver->getMode(state->ms_index); // tracked for convenience
    updateStepper();
    saveState();
  }
  return(changed);
}

bool StepperLoggerComponent::changeMicrosteppingMode(int ms_mode) {

  // find index for requested microstepping mode
  int ms_index = driver->findMicrostepIndexForMode(ms_mode);

  // no index found for requested mode
  if (ms_index == -1) {
    Serial.printf("WARNING: could not find microstep index for mode %d\n", ms_mode);
    return(false);
  }

  bool changed = state->ms_auto | (state->ms_index != ms_index);
  (changed) ?
    Serial.printf("INFO: activating microstepping index %d for mode %d\n", ms_index, ms_mode):
    Serial.printf("INFO: microstepping mode already active (%d)\n", state->ms_mode);

  if (changed) {
    // update with new microstepping mode
    state->ms_auto = false; // deactivate auto microstepping
    state->ms_index = ms_index; // set the found index
    state->ms_mode = driver->getMode(ms_index); // tracked for convenience
    setSpeedWithSteppingLimit(state->rpm); // update speed (if necessary)
    updateStepper();
    saveState();
  }
  return(changed);
}

/*** stepper functions ***/

void StepperLoggerComponent::updateStepper() {
  // update microstepping
  if (state->ms_index >= 0 && state->ms_index < driver->ms_modes_n) {
    digitalWrite(board->ms1, driver->ms_modes[state->ms_index].ms1);
    digitalWrite(board->ms2, driver->ms_modes[state->ms_index].ms2);
    digitalWrite(board->ms3, driver->ms_modes[state->ms_index].ms3);
  }

  // update speed
  stepper.setSpeed(calculateSpeed());

  // update enabled / disabled
  if (state->status == STATUS_ON || state->status == STATUS_ROTATE) {
    stepper.enableOutputs();
  } else if (state->status == STATUS_HOLD) {
    stepper.setSpeed(0);
    stepper.enableOutputs();
  } else {
    // STATUS_OFF
    stepper.setSpeed(0);
    stepper.disableOutputs();
  }

  // update data if new rpm
  float new_rpm;
  if (state->status == STATUS_ON || state->status == STATUS_ROTATE) {
    new_rpm = state->rpm * state->direction;
  } else {
    new_rpm = 0.0;
  }

  // make change if data (=rpm) not yet set or rpm has changed
  if (!data[0].newest_value_valid || fabs(new_rpm - data[0].getValue()) > 0.0001) {
   
    // save previous data in data[1] for the data log step transition
    if (data[0].newest_value_valid) {
        if (debug_mode) {
            Serial.printf("DEBUG: logging speed shift from %.4f to %.4frpm\n", data[0].getValue(), new_rpm);
        }
        data[1].setNewestDataTime(millis() - 1); // old value logged 1 ms before new value
        data[1].setNewestValue(data[0].getValue());
        data[1].saveNewestValue(false); // no averaging
    } 

    // set new value
    data[0].setNewestValue(new_rpm);

    // log data (changes data[0] time to millis())
    logData();

    // clear the step transition data[1]
    data[1].clear();

    // update whole controller data variable
    ctrl->updateDataVariable();
  }
}

float StepperLoggerComponent::calculateSpeed() {
  float speed = state->rpm/60.0 * motor->steps * motor->gearing * state->ms_mode * state->direction;
  if (debug_mode) {
    Serial.printf("DEBUG: calculated speed %.5f (micro)steps/s (%i microstep mode)\n", speed, state->ms_mode);
  }
  return(speed);
}

int StepperLoggerComponent::findMicrostepIndexForRpm(float rpm) {
  if (state->ms_auto) {
    // automatic mode --> find lowest MS mode that can handle these rpm (otherwise go to full step -> ms_index = 0)
    return(driver->findMicrostepIndexForRpm(rpm));
  } else {
    return(state->ms_index);
  }
}

bool StepperLoggerComponent::setSpeedWithSteppingLimit(float rpm) {
  if (driver->testRpmLimit(state->ms_index, rpm)) {
    state->rpm = driver->getRpmLimit(state->ms_index);
    Serial.printf("WARNING: stepping mode is not fast enough for the requested rpm: %.3f --> switching to MS mode rpm limit of %.3f\n", rpm, state->rpm);
    return(false);
  } else {
    state->rpm = rpm;
    return(true);
  }
}

/*** state changes ***/
void StepperLoggerComponent::activateDataLogging() {
  logData();
}

/*** state info to LCD display ***/

/*** logger state variable ***/

void StepperLoggerComponent::assembleStateVariable() {
  char pair[60];
  getStepperStateStatusInfo(state->status, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getStepperStateDirectionInfo(state->direction, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getStepperStateSpeedInfo(state->rpm, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getStepperStateMSInfo(state->ms_auto, state->ms_mode, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** particle webhook state log ***/

/*** logger data variable ***/

/*** particle webhook data log ***/

void StepperLoggerComponent::logData() {
  // always log data[0] with latest current time
  data[0].setNewestDataTime(millis());
  data[0].saveNewestValue(false);
  ControllerLoggerComponent::logData();
}
