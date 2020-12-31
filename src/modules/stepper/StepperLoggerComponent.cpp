#include "application.h"
#include "StepperLoggerComponent.h"

/*** state info ***/

const StepperStateInfo STATUS_INFO[] = {
   {STATUS_ON, "on", "running"},
   {STATUS_OFF, "off", "off"},
   {STATUS_HOLD, "hold", "holding position"},
   {STATUS_MANUAL, "man", "manual mode"},
   {STATUS_ROTATE, "rot", "executing number of rotations"},
   {STATUS_TRIGGER, "trig", "triggered by external signal"}
};

const StepperStateInfo DIR_INFO[] = {
   {DIR_CW, "cw", "clockwise"},
   {DIR_CC, "cc", "counter-clockwise"}
};

/*** state variable formatting ***/

// pump status info
static void getStepperStateStatusInfo(int status, char* target, int size, char* pattern, bool include_key = true)  {
  int i = 0;
  for (; i < sizeof(STATUS_INFO) / sizeof(StepperStateInfo); i++) {
    if (STATUS_INFO[i].value == status) break;
  }
  getStateStringText("status", STATUS_INFO[i].short_label, target, size, pattern, include_key);
}

static void getStepperStateStatusInfo(int status, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateStatusInfo(status, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateStatusInfo(status, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// pump direction info
static void getStepperStateDirectionInfo(int direction, char* target, int size, char* pattern, bool include_key = true)  {
  int i = 0;
  for (; i < sizeof(DIR_INFO) / sizeof(StepperStateInfo); i++) {
    if (DIR_INFO[i].value == direction) break;
  }
  getStateStringText("dir", DIR_INFO[i].short_label, target, size, pattern, include_key);
}

static void getStepperStateDirectionInfo(int direction, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateDirectionInfo(direction, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateDirectionInfo(direction, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// speed
static void getStepperStateSpeedInfo(float rpm, char* target, int size, char* pattern, bool include_key = true) {
  int decimals = find_signif_decimals(rpm, 3, true, 3); // 3 significant digits by default, max 3 after decimals
  getStateDoubleText("speed", rpm, "rpm", target, size, pattern, decimals, include_key);
}

static void getStepperStateSpeedInfo(float rpm, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateSpeedInfo(rpm, target, size, PATTERN_VU_SIMPLE, false);
  else getStepperStateSpeedInfo(rpm, target, size, PATTERN_KVU_JSON_QUOTED, true);
}

// microstepping info
static void getStepperStateMSInfo(bool ms_auto, int ms_mode, char* target, int size, char* pattern, bool include_key = true) {
  char ms_text[10];
  (ms_auto) ?
    snprintf(ms_text, sizeof(ms_text), "%dA", ms_mode) :
    snprintf(ms_text, sizeof(ms_text), "%d", ms_mode);
  getStateStringText("ms", ms_text, target, size, pattern, include_key);
}

static void getStepperStateMSInfo(bool ms_auto, int ms_mode, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateMSInfo(ms_auto, ms_mode, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateMSInfo(ms_auto, ms_mode, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/*** debug ***/
void StepperLoggerComponent::debug() {
    debug_mode = true;
}

/*** setup ***/

uint8_t StepperLoggerComponent::setupDataVector(uint8_t start_idx) { 
    start_idx = ControllerLoggerComponent::setupDataVector(start_idx);
    data.resize(2);
    // same index to allow for step transition logging
    data[0] = LoggerData(1, "speed", "rpm", 1);
    data[0].setAutoClear(false); // not cleared during auto clear
    data[1] = LoggerData(1, "speed", "rpm", 1);
    return(start_idx + 2); 
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
        Serial.println("INFO: available microstepping modes");
        for (int i = 0; i < driver->ms_modes_n; i++) {
        Serial.printf("   Mode %d: %i steps, max rpm: %.1f\n", i, driver->ms_modes[i].mode, driver->ms_modes[i].rpm_limit);
        }
    }

    //updateStepper(true); // FIXME
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

/*

void StepperLoggerComponent::construct() {
  driver->calculateRpmLimits(board->max_speed, motor->steps, motor->gearing);
  data.resize(2);
  // same index to allow for step transition logging
  data[0] = LoggerData(1, "speed", "rpm", 1);
  data[0].setAutoClear(false); // not cleared during auto clear
  data[1] = LoggerData(1, "speed", "rpm", 1);
}

void StepperLoggerComponent::init() {

  LoggerController::init();

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
  #ifdef STEPPER_DEBUG_ON
    Serial.println("INFO: available microstepping modes");
    for (int i = 0; i < driver->ms_modes_n; i++) {
      Serial.printf("   Mode %d: %i steps, max rpm: %.1f\n", i, driver->ms_modes[i].mode, driver->ms_modes[i].rpm_limit);
    }
  #endif

  updateStepper(true);
}

// loop function
void StepperLoggerComponent::update() {
  if (state->status == STATUS_ROTATE) {
    // WARNING: FIXME known bug, when power out, saved rotate status will lead to immediate stop of pump
    if (stepper.distanceToGo() == 0) {
      changeStatus(STATUS_OFF); // disengage if reached target location
      updateStateInformation();
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    stepper.runSpeed();
  }

  // log rpm once startup is complete
  if (startup_logged && !startup_rpm_logged) {
    logRpm();
    startup_rpm_logged = true;
  }

  LoggerController::update();
}


// save device state to EEPROM
void StepperLoggerComponent::saveDS() {
  EEPROM.put(STATE_ADDRESS, *state);
  #ifdef STATE_DEBUG_ON
    Serial.println("INFO: stepper state saved in memory (if any updates were necessary)");
  #endif
}

// load device state from EEPROM
bool StepperLoggerComponent::restoreDS(){
  StepperState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  bool recoverable = saved_state.version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, *state);
    Serial.printf("INFO: successfully restored state from memory (version %d)\n", STATE_VERSION);
  } else {
    Serial.printf("INFO: could not restore state from memory (found version %d), sticking with initial default\n", saved_state.version);
    saveDS();
  }
  return(recoverable);
}

void StepperLoggerComponent::updateStepper(bool init) {
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

  // log rpm (if necessary - determined in function)
  if (!init) logRpm();
}

float StepperLoggerComponent::calculateSpeed() {
  float speed = state->rpm/60.0 * motor->steps * motor->gearing * state->ms_mode * state->direction;
  #ifdef STEPPER_DEBUG_ON
    Serial.printf("INFO: calculated speed %.5f (micro)steps/s (%i microstep mode)\n", speed, state->ms_mode);
  #endif
  return(speed);
}


bool StepperLoggerComponent::changeDataLogging (bool on) {
  bool changed = LoggerController::changeDataLogging (on);
  if (on && changed) logRpm();
  return(changed);
}

bool StepperLoggerComponent::changeStatus(int status) {

  // only update if necessary
  bool changed = status != state->status;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: status updating to %d\n", status);
    else
      Serial.printf("INFO: status unchanged (%d)\n", status);
  #endif

  if (changed) {
    state->status = status;
    updateStepper();
    saveDS();
  }
  return(changed);
}

bool StepperLoggerComponent::changeDirection(int direction) {

  // only update if necessary
  bool changed = (direction == DIR_CW || direction == DIR_CC) && state->direction != direction;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      (direction == DIR_CW) ? Serial.println("INFO: changing direction to clockwise") : Serial.println("INFO: changing direction to counter clockwise");
    else
      (direction == DIR_CW) ? Serial.println("INFO: direction unchanged (clockwise)") : Serial.println("INFO: direction unchanged (counter clockwise)");
  #endif

  if (changed) {
    state->direction = direction;
    if (state->status == STATUS_ROTATE) {
      // if rotating to a specific position, changing direction turns the pump off
      #ifdef STEPPER_DEBUG_ON
        Serial.println("INFO: stepper stopped due to change in direction during 'rotate'");
      #endif
      state->status = STATUS_OFF;
    }
    updateStepper();
    saveDS();
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

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: changing rpm %.3f\n", state->rpm);
    else
      Serial.printf("INFO: rpm staying unchanged ()%.3f)\n", state->rpm);
  #endif

  if (changed) {
    updateStepper();
    saveDS();
  }
  return(changed);
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

bool StepperLoggerComponent::changeToAutoMicrosteppingMode() {

  bool changed = !state->ms_auto;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.println("INFO: activating automatic microstepping");
    else
      Serial.println("INFO: automatic microstepping already active");
  #endif

  if (changed) {
    state->ms_auto = true;
    state->ms_index = findMicrostepIndexForRpm(state->rpm);
    state->ms_mode = driver->getMode(state->ms_index); // tracked for convenience
    updateStepper();
    saveDS();
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
  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: activating microstepping index %d for mode %d\n", ms_index, ms_mode);
    else
      Serial.printf("INFO: microstepping mode already active (%d)\n", state->ms_mode);
  #endif

  if (changed) {
    // update with new microstepping mode
    state->ms_auto = false; // deactivate auto microstepping
    state->ms_index = ms_index; // set the found index
    state->ms_mode = driver->getMode(ms_index); // tracked for convenience
    setSpeedWithSteppingLimit(state->rpm); // update speed (if necessary)
    updateStepper();
    saveDS();
  }
  return(changed);
}

int StepperLoggerComponent::findMicrostepIndexForRpm(float rpm) {
  if (state->ms_auto) {
    // automatic mode --> find lowest MS mode that can handle these rpm (otherwise go to full step -> ms_index = 0)
    return(driver->findMicrostepIndexForRpm(rpm));
  } else {
    return(state->ms_index);
  }
}

// start, stop, hold
bool StepperLoggerComponent::start() { return(changeStatus(STATUS_ON)); }
bool StepperLoggerComponent::stop() { return(changeStatus(STATUS_OFF)); }
bool StepperLoggerComponent::hold() { return(changeStatus(STATUS_HOLD)); }

// number of rotations
long StepperLoggerComponent::rotate(float number) {
  long steps = state->direction * number * motor->steps * motor->gearing * state->ms_mode;
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  changeStatus(STATUS_ROTATE);
  return(steps);
}

void StepperLoggerComponent::clearData(bool all) {
  // never clear persistent data
  LoggerController::clearData(false);
}

bool StepperLoggerComponent::assembleDataLog() {
  // always reset time offset to 0
  data[0].setNewestDataTime(millis());
  data[0].saveNewestValue(false); // no averaging
  // individual time offsets
  return(LoggerController::assembleDataLog(false));
}

void StepperLoggerComponent::logRpm() {

  // new rpm
  float new_rpm;
  if (state->status == STATUS_ON || state->status == STATUS_ROTATE) {
    new_rpm = state->rpm * state->direction;
  } else {
    new_rpm = 0.0;
  }

  // log if first time call (i.e. at startup) or rpm has changed
  bool log_rpm = !data[0].newest_value_valid || fabs(new_rpm - data[0].getValue()) > 0.0001;

  // log
  if (log_rpm) {

    #ifdef DATA_DEBUG_ON
      (data[0].newest_value_valid) ?
        Serial.printf("INFO: logging speed shift from %.4f to %.4frpm\n", data[0].getValue(), new_rpm) :
        Serial.printf("INFO: re-logging speed at startup (%.4frpm)\n", new_rpm);
    #endif

    if (data[0].newest_value_valid) {
      data[1].setNewestValue(data[0].getValue());
      data[1].setNewestDataTime(millis() - 1);
      data[1].saveNewestValue(false); // no averaging
    }
    // set newet value (date time and averaging will happens assembledatalog)
    data[0].setNewestValue(new_rpm);
    data[0].setNewestDataTime(millis());
    data[0].saveNewestValue(false); // no averagings
    last_data_log = millis();
    logData();
    updateDataInformation();
    clearData(false);
  }
}


void StepperLoggerComponent::assembleStateInformation() {
  LoggerController::assembleStateInformation();
  char pair[60];
  getStepperStateStatusInfo(state->status, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateDirectionInfo(state->direction, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateSpeedInfo(state->rpm, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateMSInfo(state->ms_auto, state->ms_mode, pair, sizeof(pair)); addToStateInformation(pair);
}

void StepperLoggerComponent::updateStateInformation() {

  // state information
  LoggerController::updateStateInformation();

  // LCD update
  if (lcd) {

      // fixme: should be callback function

    // stirrer
    lcd_buffer[0] = 0; // reset buffer
    getStepperStateStatusInfo(state->status, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    snprintf(lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), " ");   
    getStepperStateSpeedInfo(state->rpm, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    snprintf(lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), " ");   
    getStepperStateDirectionInfo(state->direction, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    lcd->printLine(2, lcd_buffer);

    // pump
    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s", "Status: ");
    getStepperStateStatusInfo(state->status, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    lcd->printLine(2, lcd_buffer);

    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s", "Speed: ");
    getStepperStateSpeedInfo(state->rpm, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    lcd->printLine(3, lcd_buffer);

    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s", "Dir: ");
    getStepperStateDirectionInfo(state->direction, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    snprintf(lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), "%s", "  MS: ");
    getStepperStateMSInfo(state->ms_auto, state->ms_mode, lcd_buffer + strlen(lcd_buffer), sizeof(lcd_buffer) - strlen(lcd_buffer), true);
    lcd->printLine(4, lcd_buffer);

  }
}

bool StepperLoggerComponent::parseStatus() {
  if (command.parseVariable(CMD_START)) {
    // start
    command.success(start());
  } else if (command.parseVariable(CMD_STOP)) {
    // stop
    command.success(stop());
  } else if (command.parseVariable(CMD_HOLD)) {
    // hold
    command.success(hold());
  } else if (command.parseVariable(CMD_RUN)) {
    // run - not yet implemented FIXME
  } else if (command.parseVariable(CMD_AUTO)) {
    // auto - not yet implemented FIXME
  } else if (command.parseVariable(CMD_ROTATE)) {
    // rotate
    command.extractValue();
    char* end;
    float number = strtof (command.value, &end);
    int converted = end - command.value;
    if (converted > 0) {
      // valid number
      rotate(number);
      // rotate always counts as new command b/c rotation starts from scratch
      command.success(true);
    } else {
      // no number, invalid value
      command.errorValue();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateStatusInfo(state->status, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperLoggerComponent::parseDirection() {

  if (command.parseVariable(CMD_DIR)) {
    // direction
    command.extractValue();
    if (command.parseValue(CMD_DIR_CW)) {
      // clockwise
      command.success(changeDirection(DIR_CW));
    } else if (command.parseValue(CMD_DIR_CC)) {
      // counter clockwise
      command.success(changeDirection(DIR_CC));
    } else if (command.parseValue(CMD_DIR_SWITCH)) {
      // switch
      command.success(changeDirection(-state->direction));
    } else {
      // invalid
      command.errorValue();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateDirectionInfo(state->direction, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperLoggerComponent::parseSpeed() {

  if (command.parseVariable(CMD_SPEED)) {
    // speed
    command.extractValue();
    command.extractUnits();

    if (command.parseUnits(SPEED_RPM)) {
      // speed rpm
      char* end;
      float number = strtof (command.value, &end);
      int converted = end - command.value;
      if (converted > 0) {
        // valid number
        command.success(changeSpeedRpm(number));
        if( (state->rpm - number) < 0.0 ) {
          // could not set to rpm, hit the max --> set warning
          command.warning(CMD_RET_WARN_MAX_RPM, CMD_RET_WARN_MAX_RPM_TEXT);
        }
      } else {
        // no number, invalid value
        command.errorValue();
      }
    //} else if (command.parseUnits(SPEED_FPM)) {
      // speed fpm
      // TODO
    } else {
      command.errorUnits();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateSpeedInfo(state->rpm, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperLoggerComponent::parseMS() {

  if (command.parseVariable(CMD_STEP)) {
    // microstepping
    command.extractValue();
    if (command.parseValue(CMD_STEP_AUTO)) {
      command.success(changeToAutoMicrosteppingMode());
    } else {
      int ms_mode = atoi(command.value);
      command.success(changeMicrosteppingMode(ms_mode));
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateMSInfo(state->ms_auto, state->ms_mode, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

void StepperLoggerComponent::parseCommand() {

  LoggerController::parseCommand();

  if (command.isTypeDefined()) {
    // command processed successfully by parent function
  } else if (parseStatus()) {
    // check for status commands
  } else if (parseDirection()) {
    // check for direction commands
  } else if (parseSpeed()) {
    // check for speed commands
  //} else if (parseRamp()) {
    // check for ramp commands - TODO
  } else if (parseMS()) {
    // check for microstepping commands
  }

}

*/