#include "application.h"
#include "LoggerController.h"
#include "LoggerComponent.h"

// EEPROM variabls
#define STATE_ADDRESS    0 // EEPROM storage location
#define EEPROM_START    0 // EEPROM storage start
const size_t EEPROM_MAX = EEPROM.length();

/* COMPONENTS */

// add component NEW
void LoggerController::addComponent(LoggerComponent* component) {
    component->setEEPROMStart(eeprom_location);
    eeprom_location = eeprom_location + component->getStateSize();
    data_idx = component->setupDataVector(data_idx);
    if (eeprom_location >= EEPROM_MAX) {
    Serial.printf("ERROR: component '%s' state would exceed EEPROM size, cannot add component.\n", component->id);
    } else {
    Serial.printf("INFO: adding component '%s' to the controller.\n", component->id);
    components.push_back(component);
    }
}

/* STATE MANAGEMENT */

void LoggerController::loadState(bool reset)
{
  if (!reset)
  {
    Serial.printf("INFO: trying to restore state from memory for controller '%s'\n", version);
    restoreState();
  }
  else
  {
    Serial.printf("INFO: resetting state for controller '%s' back to default values\n", version);
    saveState();
  }
};

void LoggerController::loadComponentsState(bool reset)
{
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->loadState(reset);
  }
}

void LoggerController::saveState()
{
  EEPROM.put(eeprom_start, *state);
#ifdef STATE_DEBUG_ON
  Serial.printf("INFO: controller '%s' state saved in memory (if any updates were necessary)\n", version);
#endif
};

bool LoggerController::restoreState()
{
  LoggerControllerState *saved_state = new LoggerControllerState();
  EEPROM.get(eeprom_start, *saved_state);
  bool recoverable = saved_state->version == state->version;
  if (recoverable)
  {
    EEPROM.get(eeprom_start, *state);
    Serial.printf("INFO: successfully restored controller state from memory (state version %d)\n", state->version);
  }
  else
  {
    Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
    saveState();
  }
  return (recoverable);
};

/* INIT */

void LoggerController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // initialize
  Serial.printf("INFO: initializing controller '%s'...\n", version);

  // lcd
  lcd->init();
  lcd->printLine(1, version);

  //  check for reset
  if(digitalRead(reset_pin) == HIGH) {
    reset = true;
    Serial.println("INFO: reset request detected");
    lcd->printLineTemp(1, "Resetting...");
  }

  // controller state
  loadState(reset);
  loadComponentsState(reset);

  // components' init
  initComponents();

  // startup time info
  Serial.println(Time.format(Time.now(), "INFO: startup time: %Y-%m-%d %H:%M:%S %Z"));

  // state and log variables
  strcpy(state_variable, "{}");
  state_variable[2] = 0;
  strcpy(data_information, "{}");
  data_information[2] = 0;
  strcpy(state_log, "{}");
  state_log[2] = 0;
  strcpy(data_log, "{}");
  data_log[2] = 0;

  // register particle functions
  Serial.println("INFO: registering logger cloud variables");
  Particle.subscribe("spark/", &LoggerController::captureName, this, MY_DEVICES);
  Particle.function(CMD_ROOT, &LoggerController::receiveCommand, this);
  Particle.variable(STATE_INFO_VARIABLE, state_variable);
  Particle.variable(DATA_INFO_VARIABLE, data_information);
  #ifdef WEBHOOKS_DEBUG_ON
    // report logs in variables instead of webhooks
    Particle.variable(STATE_LOG_WEBHOOK, state_log);
    Particle.variable(DATA_LOG_WEBHOOK, data_log);
  #endif

  // data log
  last_data_log = 0;
}

void LoggerController::initComponents() 
{
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->init();
  }
}

/* DATA UPDATES */

bool LoggerController::isTimeForDataLogAndClear() {

  if (state->data_logging_type == LOG_BY_TIME) {
    // go by time
    unsigned long log_period = state->data_logging_period * 1000;
    if ((millis() - last_data_log) > log_period) {
      #ifdef DATA_DEBUG_ON
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.printf("INFO: triggering data log at %s (after %d seconds)\n", date_time_buffer, state->data_logging_period);
      #endif
      return(true);
    }
  } else if (state->data_logging_type == LOG_BY_EVENT) {
    /*
    // not implemented! this needs to be handled by each component individually so requires a mechanism for components to trigger a specific partial data log
    // go by read number
    if (data[0].getN() >= state->data_logging_period) {
      #ifdef DATA_DEBUG_ON
      Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
      Serial.printf("INFO: triggering data log at %s (after %d reads)\n", date_time_buffer, state->data_logging_period);
      #endif
      return(true);
    }
    */
    Serial.println("ERROR: LOG_BY_EVENT data logging type is not yet implemented!");
  } else {
    Serial.printf("ERROR: unknown logging type stored in state - this should be impossible! %d\n", state->data_logging_type);
  }
  return(false);
}

/* UPDATE */

void LoggerController::update() {

  // cloud connection
	if (Particle.connected()) {
		if (!cloud_connected) {
      // connection freshly made
      WiFi.macAddress(mac_address);
      Serial.printf("INFO: MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", 
        mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
      Serial.println(Time.format(Time.now(), "INFO: cloud connection established at %H:%M:%S %d.%m.%Y"));
      cloud_connected = true;
      lcd->printLine(2, ""); // clear "connect wifi" message
      updateDisplayStateInformation();
      updateDisplayComponentsStateInformation();
      // name capture
      if (!name_handler_registered){
        name_handler_registered = Particle.publish("spark/device/name", PRIVATE);
        if (name_handler_registered) Serial.println("INFO: name handler registered");
      }
		}
		Particle.process();
	} else if (cloud_connected) {
		// should be connected but isn't --> reconnect
		Serial.println(Time.format(Time.now(), "INFO: lost cloud connection at %H:%M:%S %d.%m.%Y"));
		cloud_connection_started = false;
		cloud_connected = false;
	} else if (!cloud_connection_started) {
		// start cloud connection
		Serial.println(Time.format(Time.now(), "INFO: initiate cloud connection at %H:%M:%S %d.%m.%Y"));
    lcd->printLine(2, "Connect WiFi...");
    updateDisplayStateInformation(); // not components, preserve connect wifi message
		Particle.connect();
		cloud_connection_started = true;
	}

  // startup complete
  if (Particle.connected() && !startup_logged && name_handler_succeeded) {
    // state and data information
    updateLoggerStateVariable();
    updateDataInformation();
    if (state->state_logging) {
      Serial.println("INFO: start-up completed.");
      assembleStartupLog();
      publishStateLog();
    } else {
      Serial.println("INFO: start-up completed (not logged).");
    }
    startup_logged = true;
  }

  // data reset
  if (isTimeForDataLogAndClear()) {

    // make note for last data log
    last_data_log = millis();
    logData();
    clearData(false);
  }

  // components update
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
     (*components_iter)->update();
  }

  // lcd update
  lcd->update();

}

/* Logger NAME */

void LoggerController::captureName(const char *topic, const char *data) {
  // store name and also assign it to Logger information
  strncpy ( name, data, sizeof(name) );
  name_handler_succeeded = true;
  Serial.println("INFO: logger name '" + String(name) + "'");
  lcd->printLine(1, name);
  if (name_callback) name_callback();
}

void LoggerController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

/* Logger STATE CHANGE FUNCTIONS */

// locking
bool LoggerController::changeLocked(bool on) {
  bool changed = on != state->locked;

  if (changed) {
    state->locked = on;
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: locking Logger") : Serial.println("INFO: unlocking Logger");
    else
      on ? Serial.println("INFO: Logger already locked") : Serial.println("INFO: Logger already unlocked");
  #endif

  if (changed) saveState();

  return(changed);
}

// state log
bool LoggerController::changeStateLogging (bool on) {
  bool changed = on != state->state_logging;

  if (changed) {
    state->state_logging = on;
    override_state_log = true; // always log this event no matter what
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
    else
      on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
  #endif

  if (changed) saveState();

  return(changed);
}

// data log
bool LoggerController::changeDataLogging (bool on) {
  bool changed = on != state->data_logging;

  if (changed) {
    state->data_logging = on;
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: data logging turned on") : Serial.println("INFO: data logging turned off");
    else
      on ? Serial.println("INFO: data logging already on") : Serial.println("INFO: data logging already off");
  #endif

  if (changed) saveState();

  // make sure all data is cleared
  if (changed && on) clearData(true);

  return(changed);
}

// logging period
bool LoggerController::changeDataLoggingPeriod(int period, int type) {
  bool changed = period != state->data_logging_period | type != state->data_logging_type;

  if (changed) {
    state->data_logging_period = period;
    state->data_logging_type = type;
  }

  #ifdef STATE_DEBUG_ON
    if (changed) Serial.printf("INFO: setting data logging period to %d %s\n", period, type == LOG_BY_TIME ? "seconds" : "reads");
    else Serial.printf("INFO: data logging period unchanged (%d)\n", type == LOG_BY_TIME ? "seconds" : "reads");
  #endif

  if (changed) saveState();

  return(changed);
}

// reading period
bool LoggerController::changeDataReadingPeriod(int period) {

  // safety check (should never get here)
  if (!state->data_reader) {
    Serial.println("ERROR: not a data reader! cannot change reading period.");
    return(false);
  }

  bool changed = period != state->data_reading_period;

  if (changed) {
    state->data_reading_period = period;
  }

  #ifdef STATE_DEBUG_ON
    if (changed) Serial.printf("INFO: setting data reading period to %d ms\n", period);
    else Serial.printf("INFO: data reading period unchanged (%d ms)\n", period);
  #endif

  if (changed) saveState();

  return(changed);
}

/* COMMAND PARSING FUNCTIONS */

void LoggerController::setCommandCallback(void (*cb)()) {
  command_callback = cb;
}

bool LoggerController::parseLocked() {
  // decision tree
  if (command->parseVariable(CMD_LOCK)) {
    // locking
    command->extractValue();
    if (command->parseValue(CMD_LOCK_ON)) {
      command->success(changeLocked(true));
    } else if (command->parseValue(CMD_LOCK_OFF)) {
      command->success(changeLocked(false));
    }
    getStateLockedText(state->locked, command->data, sizeof(command->data));
  } else if (state->locked) {
    // Logger is locked --> no other commands allowed
    command->errorLocked();
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseStateLogging() {
  if (command->parseVariable(CMD_STATE_LOG)) {
    // state logging
    command->extractValue();
    if (command->parseValue(CMD_STATE_LOG_ON)) {
      command->success(changeStateLogging(true));
    } else if (command->parseValue(CMD_STATE_LOG_OFF)) {
      command->success(changeStateLogging(false));
    }
    getStateStateLoggingText(state->state_logging, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataLogging() {
  if (command->parseVariable(CMD_DATA_LOG)) {
    // state logging
    command->extractValue();
    if (command->parseValue(CMD_DATA_LOG_ON)) {
      command->success(changeDataLogging(true));
    } else if (command->parseValue(CMD_DATA_LOG_OFF)) {
      command->success(changeDataLogging(false));
    }
    getStateDataLoggingText(state->data_logging, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseReset() {
  if (command->parseVariable(CMD_RESET)) {
    command->extractValue();
    if (command->parseValue(CMD_RESET_DATA)) {
      clearData(true); // clear all data
      command->success(true);
      getStateStringText(CMD_RESET, CMD_RESET_DATA, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
    } else  if (command->parseValue(CMD_RESET_STATE)) {
      state->version = 0; // force reset of state on next startup
      saveState();
      command->success(true);
      getStateStringText(CMD_RESET, CMD_RESET_STATE, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
      command->setLogMsg("reset state on next startup");
    }
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataLoggingPeriod() {
  if (command->parseVariable(CMD_DATA_LOG_PERIOD)) {
    // parse read period
    command->extractValue();
    int log_period = atoi(command->value);
    if (log_period > 0) {
      command->extractUnits();
      uint8_t log_type = LOG_BY_TIME;
      if (command->parseUnits(CMD_DATA_LOG_PERIOD_NUMBER)) {
        // events
        log_type = LOG_BY_EVENT;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_SEC)) {
        // seconds (the base unit)
        log_period = log_period;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_MIN)) {
        // minutes
        log_period = 60 * log_period;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_HR)) {
        // minutes
        log_period = 60 * 60 * log_period;
      } else {
        // unrecognized units
        command->errorUnits();
      }
      // assign read period
      if (!command->isTypeDefined()) {
        if (log_type == LOG_BY_EVENT || (log_type == LOG_BY_TIME && (log_period * 1000) > state->data_reading_period))
          command->success(changeDataLoggingPeriod(log_period, log_type));
        else
          // make sure smaller than log period
          command->error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
      }
    } else {
      // invalid value
      command->errorValue();
    }
    getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataReadingPeriod() {
  if (command->parseVariable(CMD_DATA_READ_PERIOD)) {

    // parse read period
    command->extractValue();

    if(!state->data_reader) {
      // not actually a data reader
      command->error(CMD_RET_ERR_NOT_A_READER, CMD_RET_ERR_NOT_A_READER_TEXT);
    } else if (command->parseValue(CMD_DATA_READ_PERIOD_MANUAL)){
      // manual reads
      command->success(changeDataReadingPeriod(READ_MANUAL));
    } else {
      // specific read period
      int read_period = atoi(command->value);
      if (read_period > 0) {
        command->extractUnits();
        if (command->parseUnits(CMD_DATA_READ_PERIOD_MS)) {
          // milli seconds (the base unit)
          read_period = read_period;
        } else if (command->parseUnits(CMD_DATA_READ_PERIOD_SEC)) {
          // seconds
          read_period = 1000 * read_period;
        } else if (command->parseUnits(CMD_DATA_READ_PERIOD_MIN)) {
          // minutes
          read_period = 1000 * 60 * read_period;
        } else {
          // unrecognized units
          command->errorUnits();
        }
        // assign read period
        if (!command->isTypeDefined()) {
          if (read_period < state->data_reading_period_min)
            // make sure bigger than minimum
            command->error(CMD_RET_ERR_READ_LARGER_MIN, CMD_RET_ERR_READ_LARGER_MIN_TEXT);
          else if (state->data_logging_type == LOG_BY_TIME && state->data_logging_period * 1000 <= read_period)
            // make sure smaller than log period
            command->error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
          else
            command->success(changeDataReadingPeriod(read_period));
        }
      } else {
        // invalid value
        command->errorValue();
      }
    }

    // include current read period in data
    if(state->data_reader) {
      getStateDataReadingPeriodText(state->data_reading_period, command->data, sizeof(command->data));
    }
  }
  return(command->isTypeDefined());
}

/****** WEB COMMAND PROCESSING *******/

int LoggerController::receiveCommand(String command_string) {

  // load, parse and finalize command
  command->load(command_string);
  command->extractVariable();
  parseCommand();

  // mark error if type still undefined
  if (!command->isTypeDefined()) command->errorCommand();

  // lcd info
  updateDisplayCommandInformation();

  // assemble and publish log
  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("INFO: webhook debugging is on --> always assemble state log and publish to variable");
    override_state_log = true;
  #endif
  if (state->state_logging | override_state_log) {
    assembleStateLog();
    publishStateLog();
  }
  override_state_log = false;

  // state information
  if (command->hasStateChanged()) {
    updateLoggerStateVariable();
  }

  // command reporting callback
  if (command_callback) command_callback();

  // return value
  return(command->ret_val);
}

void LoggerController::parseCommand() {

  // decision tree
  if (parseLocked()) {
    // locked is getting parsed
  } else if (parseStateLogging()) {
    // state logging getting parsed
  } else if (parseDataLogging()) {
    // data logging getting parsed
  } else if (parseDataLoggingPeriod()) {
    // parsing logging period
  } else if (parseDataReadingPeriod()) {
    // parsing reading period
  } else if (parseReset()) {
    // reset getting parsed
  } else {
    parseComponentsCommand();
  }

}

void LoggerController::parseComponentsCommand() {
  bool success = false;
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     success = (*components_iter)->parseCommand(command);
     if (success) break;
  }
}

/* COMMAND DISPLAY INFORMATION */

void LoggerController::updateDisplayCommandInformation() {
  assembleDisplayCommandInformation();
  showDisplayCommandInformation();
}

void LoggerController::assembleDisplayCommandInformation() {
  if (command->ret_val == CMD_RET_ERR_LOCKED)
    // make user aware of locked status since this may be a confusing error
    snprintf(lcd_buffer, sizeof(lcd_buffer), "LOCK%s: %s", command->type_short, command->command);
  else
    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s: %s", command->type_short, command->command);
}

void LoggerController::showDisplayCommandInformation() {
  lcd->printLineTemp(1, lcd_buffer);
}

/* STATE DISPLAY INFORMATION */

void LoggerController::updateDisplayStateInformation() {
  lcd_buffer[0] = 0; // reset buffer
  assembleDisplayStateInformation();
  showDisplayStateInformation();
}

void LoggerController::assembleDisplayStateInformation() {
  uint i = 0;
  if (Particle.connected()) {
    lcd_buffer[i] = 'W';
  } else {
    lcd_buffer[i] = '!';
  }
  i++;
  if (state->locked) {
    lcd_buffer[i] = 'L';
    i++;
  }
  if (state->state_logging) {
    // state logging
    lcd_buffer[i] = 'S';
    i++;
  }
  if (state->data_logging) {
    // data logging
    lcd_buffer[i] = 'D';
    i++;
    getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type,
        lcd_buffer + i, sizeof(lcd_buffer) - i, true);
    i = strlen(lcd_buffer);
  }

  // data reading period
  if (state->data_reader) {
    lcd_buffer[i] = 'R'; 
    i++;
    if (state->data_reading_period == READ_MANUAL) {
      lcd_buffer[i] = 'M';
      i++;
    } else {
      getStateDataReadingPeriodText(state->data_reading_period, 
        lcd_buffer + i, sizeof(lcd_buffer) - i, true);
      i = strlen(lcd_buffer);
    }
  }

  lcd_buffer[i] = 0;
}

void LoggerController::showDisplayStateInformation() {
  if (name_handler_succeeded)
    lcd->printLine(1, name);
  lcd->printLineRight(1, lcd_buffer, strlen(lcd_buffer) + 1);
}

/* STATE INFORMATION */

void LoggerController::updateLoggerStateVariable() {
  #ifdef CLOUD_DEBUG_ON
    Serial.print("INFO: updating state variable: ");
  #endif
  updateDisplayStateInformation();
  updateDisplayComponentsStateInformation();
  state_variable_buffer[0] = 0; // reset buffer
  assembleLoggerStateVariable();
  assembleLoggerComponentsStateVariable();
  postLoggerStateVariable();
  #ifdef CLOUD_DEBUG_ON
    Serial.println(state_variable);
  #endif
}

void LoggerController::updateDisplayComponentsStateInformation() {
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     (*components_iter)->updateDisplayStateInformation();
  }
}

void LoggerController::assembleLoggerStateVariable() {
  char pair[60];
  getStateLockedText(state->locked, pair, sizeof(pair)); addToLoggerStateVariableBuffer(pair);
  getStateStateLoggingText(state->state_logging, pair, sizeof(pair)); addToLoggerStateVariableBuffer(pair);
  getStateDataLoggingText(state->data_logging, pair, sizeof(pair)); addToLoggerStateVariableBuffer(pair);
  getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type, pair, sizeof(pair)); addToLoggerStateVariableBuffer(pair);
  if (state->data_reader) {
    getStateDataReadingPeriodText(state->data_reading_period, pair, sizeof(pair)); addToLoggerStateVariableBuffer(pair);
  }
}

void LoggerController::assembleLoggerComponentsStateVariable() {
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     (*components_iter)->assembleLoggerStateVariable();
  }
}

void LoggerController::addToLoggerStateVariableBuffer(char* info) {
  if (state_variable_buffer[0] == 0) {
    strncpy(state_variable_buffer, info, sizeof(state_variable_buffer));
  } else {
    snprintf(state_variable_buffer, sizeof(state_variable_buffer),
        "%s,%s", state_variable_buffer, info);
  }
}

void LoggerController::postLoggerStateVariable() {
  if (Particle.connected()) {
    Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
    // dt = datetime, s = state information
    snprintf(state_variable, sizeof(state_variable), 
      "{\"dt\":\"%s\",\"version\":\"%s\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"s\":[%s]}",
      date_time_buffer, version, 
      mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5],
      state_variable_buffer);
  } else {
    Serial.println("ERROR: particle not (yet) connected.");
  }
}

/* DATA RESET */

void LoggerController::clearData(bool all) {
  // clear data for components
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
     (*components_iter)->clearData(all);
  }
}

/* DATA INFORMATION */

void LoggerController::updateDataInformation() {
  #ifdef CLOUD_DEBUG_ON
    Serial.print("INFO: updating data information: ");
  #endif
  data_information_buffer[0] = 0; // reset buffer
  assembleDataInformation();
  if (Particle.connected()) {
    postDataInformation();
  } else {
    Serial.println("ERROR: particle not (yet) connected.");
  }
  #ifdef CLOUD_DEBUG_ON
    Serial.println(data_information);
  #endif
  if (data_callback) data_callback();
}

void LoggerController::postDataInformation() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, d = structured data
  snprintf(data_information, sizeof(data_information), "{\"dt\":\"%s\",\"d\":[%s]}",
    date_time_buffer, data_information_buffer);
}

void LoggerController::addToDataInformation(char* info) {
  if (data_information_buffer[0] == 0) {
    strncpy(data_information_buffer, info, sizeof(data_information_buffer));
  } else {
    snprintf(data_information_buffer, sizeof(data_information_buffer),
        "%s,%s", data_information_buffer, info);
  }
}

void LoggerController::assembleDataInformation() {
  for (int i=0; i<data.size(); i++) {
    data[i].assembleInfo();
    addToDataInformation(data[i].json);
  }
}

void LoggerController::setDataCallback(void (*cb)()) {
  data_callback = cb;
}

/***** DATA LOG *****/

void LoggerController::logData() {
  // publish data log
  bool override_data_log = false;
  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("INFO: webhook debugging is on --> always assemble data log and publish to variable");
    override_data_log = true;
  #endif
  if (state->data_logging | override_data_log) {
      // log data for components
      for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
        (*components_iter)->logData();
      }
  } else {
    #ifdef CLOUD_DEBUG_ON
      Serial.println("INFO: data log is turned off --> continue without logging");
    #endif
  }
}

void LoggerController::resetDataLog() {
  data_log[0] = 0;
  data_log_buffer[0] = 0;
}

bool LoggerController::addToDataLogBuffer(char* info) {

  // characters reserved for rest of data log
  const uid_t reserve = 50;
  if (strlen(data_log_buffer) + strlen(info) + reserve >= sizeof(data_log)) {
    // not enough space in the data log to add more to the buffer
    return(false);
  }

  // still enough space
  if (data_log_buffer[0] == 0) {
    // buffer empty, start from scract
    strncpy(data_log_buffer, info, sizeof(data_log_buffer));
  } else {
    // concatenate existing buffer with new info
    snprintf(data_log_buffer, sizeof(data_log_buffer),
        "%s,%s", data_log_buffer, info);
  }
  return(true);
}

bool LoggerController::finalizeDataLog(bool use_common_time, unsigned long common_time) {
  // data
  int buffer_size;
  if (use_common_time) {
    // id = Logger name, to = time offset (global), d = structured data
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"to\":%lu,\"d\":[%s]}", name, common_time, data_log_buffer);
  } else {
    // indivudal time
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"d\":[%s]}", name, data_log_buffer);
  }
  if (buffer_size < 0 || buffer_size >= sizeof(data_log)) {
    Serial.println("ERROR: data log buffer not large enough for data log - this should NOT be possible to happen");
    lcd->printLineTemp(1, "ERR: datalog too big");
    return(false);
  }
  return(true);
}

bool LoggerController::publishDataLog() {

  #ifdef CLOUD_DEBUG_ON
    if (!state->data_logging) Serial.println("WARNING: publishing data log despite data logging turned off");
    Serial.printf("INFO: publishing data log '%s' to event '%s'... ", data_log, DATA_LOG_WEBHOOK);
    #ifdef WEBHOOKS_DEBUG_ON
      Serial.println();
    #endif
  #endif

  if (strlen(data_log) == 0) {
    Serial.println("WARNING: no data log sent because there is none.");
    return(false);
  }

  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("WARNING: data log NOT sent because in WEBHOOKS_DEBUG_ON mode.");
    return(false);
  #else
    bool success = Particle.connected() && Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK);

    #ifdef CLOUD_DEBUG_ON
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    #endif

    (success) ?
        lcd->printLineTemp(1, "INFO: data log sent") :
        lcd->printLineTemp(1, "ERR: data log error");

    return(success);
  #endif
}

/***** STATE LOG ******/

void LoggerController::assembleStartupLog() {
  // id = Logger name, t = state log type, s = state change, m = message, n = notes
  snprintf(state_log, sizeof(state_log), "{\"id\":\"%s\",\"t\":\"startup\",\"s\":[{\"k\":\"startup\",\"v\":\"complete\"}],\"m\":\"\",\"n\":\"\"}", name);
}

void LoggerController::assembleStateLog() {
  state_log[0] = 0;
  if (command->data[0] == 0) strcpy(command->data, "{}"); // empty data entry
  // id = Logger name, t = state log type, s = state change, m = message, n = notes
  int buffer_size = snprintf(state_log, sizeof(state_log),
     "{\"id\":\"%s\",\"t\":\"%s\",\"s\":[%s],\"m\":\"%s\",\"n\":\"%s\"}",
     name, command->type, command->data, command->msg, command->notes);
  if (buffer_size < 0 || buffer_size >= sizeof(state_log)) {
    Serial.println("ERROR: state log buffer not large enough for state log");
    lcd->printLineTemp(1, "ERR: statelog too big");
    // FIXME: implement better size checks!!, i.e. split up call --> malformatted JSON will crash the webhook
  }
}

bool LoggerController::publishStateLog() {
  #ifdef CLOUD_DEBUG_ON
    Serial.print("INFO: publishing state log " + String(state_log) + " to event '" + String(STATE_LOG_WEBHOOK) + "'... ");
    #ifdef WEBHOOKS_DEBUG_ON
      Serial.println();
    #endif
  #endif

  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("WARNING: state log NOT sent because in WEBHOOKS_DEBUG_ON mode.");
  #else
    bool success = Particle.connected() && Particle.publish(STATE_LOG_WEBHOOK, state_log, PRIVATE, WITH_ACK);

    #ifdef CLOUD_DEBUG_ON
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    #endif

    return(success);
  #endif
}
