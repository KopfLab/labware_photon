#include "application.h"
#include "LoggerController.h"
#include "LoggerComponent.h"
#include "LoggerUtils.h"

// EEPROM variables
#define STATE_ADDRESS    0 // EEPROM storage location
#define EEPROM_START    0  // EEPROM storage start
const size_t EEPROM_MAX = EEPROM.length();

/*** state variable formatting ***/

// locked text
static void getStateLockedText(bool locked, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_LOCK, locked, CMD_LOCK_ON, CMD_LOCK_OFF, target, size, pattern, include_key);
}
static void getStateLockedText(bool locked, char* target, int size, bool value_only = false) {
  if (value_only) getStateLockedText(locked, target, size, PATTERN_V_SIMPLE, false);
  else getStateLockedText(locked, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// state logging
static void getStateStateLoggingText(bool state_logging, char* target, int size, char* pattern, bool include_key = true, bool debug_webhooks = false) {
  if (debug_webhooks) {
    getStateStringText(CMD_STATE_LOG, "debug", target, size, pattern, include_key);
  } else {
    getStateBooleanText(CMD_STATE_LOG, state_logging, CMD_STATE_LOG_ON, CMD_STATE_LOG_OFF, target, size, pattern, include_key);
  }
}

static void getStateStateLoggingText(bool state_logging, char* target, int size, bool value_only = false, bool debug_webhooks = false) {
  if (value_only) getStateStateLoggingText(state_logging, target, size, PATTERN_V_SIMPLE, false, debug_webhooks);
  else getStateStateLoggingText(state_logging, target, size, PATTERN_KV_JSON_QUOTED, true, debug_webhooks);
}

// data logging
static void getStateDataLoggingText(bool data_logging, char* target, int size, char* pattern, bool include_key = true, bool debug_webhooks = false) {
  if (debug_webhooks) {
    getStateStringText(CMD_DATA_LOG, "debug", target, size, pattern, include_key);
  } else {
    getStateBooleanText(CMD_DATA_LOG, data_logging, CMD_DATA_LOG_ON, CMD_DATA_LOG_OFF, target, size, pattern, include_key);
  }
}

static void getStateDataLoggingText(bool data_logging, char* target, int size, bool value_only = false, bool debug_webhooks = false) {
  if (value_only) getStateDataLoggingText(data_logging, target, size, PATTERN_V_SIMPLE, false, debug_webhooks);
  else getStateDataLoggingText(data_logging, target, size, PATTERN_KV_JSON_QUOTED, true, debug_webhooks);
}

// data logging period (any pattern)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, char* pattern, bool include_key = true) {
  // specific logging period
  char units[] = "?";
  if (logging_type == LOG_BY_EVENT) {
    // by read
    strcpy(units, CMD_DATA_LOG_PERIOD_NUMBER);
  } else  if (logging_type == LOG_BY_TIME) {
    // by time
    if (logging_period % 3600 == 0) {
      // hours
      strcpy(units, "h");
      logging_period = logging_period/3600;
    } else if (logging_period % 60 == 0) {
      // minutes
      strcpy(units, "m");
      logging_period = logging_period/60;

    } else {
      strcpy(units, "s");
    }
  } else {
    // not supported!
    Serial.printf("ERROR: unknown logging type %d\n", logging_type);
  }

  getStateIntText(CMD_DATA_LOG_PERIOD, logging_period, units, target, size, pattern, include_key);
}

// logging period (standard patterns)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, bool value_only = false) {
  if (value_only) {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_VU_SIMPLE, false);
  } else {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_KVU_JSON, true);
  }
}

// logging_period (any pattern)
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, char* pattern, bool include_key = true) {
  if (reading_period == 0) {
    // manual mode
    getStateStringText(CMD_DATA_READ_PERIOD, CMD_DATA_READ_PERIOD_MANUAL, target, size, pattern, include_key);
  } else {
    // specific reading period
    char units[] = "ms";
    if (reading_period % 60000 == 0) {
      // minutes
      strcpy(units, "m");
      reading_period = reading_period/60000;
    } else if (reading_period % 1000 == 0) {
      // seconds
      strcpy(units, "s");
      reading_period = reading_period/1000;
    }
    getStateIntText(CMD_DATA_READ_PERIOD, reading_period, units, target, size, pattern, include_key);
  }
}

// read period (standard patterns)
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, bool value_only = false) {
  if (value_only) {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_V_SIMPLE, false) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_VU_SIMPLE, false); // number
  } else {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KV_JSON_QUOTED, true) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KVU_JSON, true); // number
  }
}

/*** watchdog ***/

static void watchdogHandler() {
  System.reset(RESET_WATCHDOG, RESET_NO_WAIT);
}

/*** debugs ***/

void LoggerController::debugCloud(){
  debug_cloud = true;
}

void LoggerController::debugWebhooks(){
  debug_webhooks = true;
}

void LoggerController::debugState(){
  debug_state = true;
}

void LoggerController::debugData(){
  debug_data = true;
}

void LoggerController::debugDisplay() {
  lcd->debug();
}

void LoggerController::forceReset() {
  reset = true;
}

/*** callbacks ***/

void LoggerController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

void LoggerController::setCommandCallback(void (*cb)()) {
  command_callback = cb;
}

void LoggerController::setStateUpdateCallback(void (*cb)()) {
  state_update_callback = cb;
}

void LoggerController::setDataUpdateCallback(void (*cb)()) {
  data_update_callback = cb;
}

/*** setup ***/

void LoggerController::addComponent(LoggerComponent* component) {
    component->setEEPROMStart(eeprom_location);
    eeprom_location = eeprom_location + component->getStateSize();
    data_idx = component->setupDataVector(data_idx);
    if (debug_data) {
      for(int i = 0; i < component->data.size(); i++) {
        component->data[i].debug();
      }
    }
    if (eeprom_location >= EEPROM_MAX) {
      Serial.printf("ERROR: component '%s' state would exceed EEPROM size, cannot add component.\n", component->id);
    } else {
      Serial.printf("INFO: adding component '%s' to the controller.\n", component->id);
      components.push_back(component);
    }
}

void LoggerController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // initialize
  Serial.printf("INFO: initializing controller '%s'...\n", version);

  // capturing system reset information
  if (System.resetReason() == RESET_REASON_USER) {
      past_reset = System.resetReasonData();
      if (past_reset == RESET_RESTART) {
        Serial.println("INFO: restarting per user request");
      } else if (past_reset == RESET_STATE) {
        Serial.println("INFO: restarting for state reset");
      } else if (past_reset == RESET_WATCHDOG) {
        Serial.println("WARNING: restarting because of watchdog");
      }
  }

  // starting application watchdog
  System.enableFeature(FEATURE_RESET_INFO);
  Serial.println("INFO: starting application watchdog");
  wd = new ApplicationWatchdog(60s, watchdogHandler, 1536);

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
  strcpy(data_variable, "{}");
  data_variable[2] = 0;
  strcpy(state_log, "{}");
  state_log[2] = 0;
  strcpy(data_log, "{}");
  data_log[2] = 0;

  // register particle functions
  Serial.println("INFO: registering logger cloud variables");
  Particle.subscribe("spark/", &LoggerController::captureName, this, MY_DEVICES);
  Particle.function(CMD_ROOT, &LoggerController::receiveCommand, this);
  Particle.variable(STATE_INFO_VARIABLE, state_variable);
  Particle.variable(DATA_INFO_VARIABLE, data_variable);
  if (debug_webhooks) {
    // report logs in variables instead of webhooks
    Particle.variable(STATE_LOG_WEBHOOK, state_log);
    Particle.variable(DATA_LOG_WEBHOOK, data_log);
  }

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

void LoggerController::completeStartup() {
  // update state and data information now that name is available
  updateStateVariable();
  updateDataVariable();

  if (state->state_logging) {
    Serial.println("INFO: start-up completed.");
    assembleStartupLog();
    publishStateLog();
  } else {
    Serial.println("INFO: start-up completed (not logged).");
  }

  // complete components' startup
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->completeStartup();
  }
}

/*** loop ***/

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

            // update display
            updateDisplayStateInformation();
            updateDisplayComponentsStateInformation();
            if (state_update_callback) state_update_callback();

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

    // startup complete once name handler succeeds (could be some time after initial particle connect)
    if (Particle.connected() && !startup_logged && name_handler_succeeded) {
       completeStartup();
       startup_logged = true;
    }

    // components update
    for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
        (*components_iter)->update();
    }

    // lcd update
    lcd->update();

    // time to log data?
    if (isTimeForDataLogAndClear()) {
        last_data_log = millis();
        logData();
        clearData(false);
    }

    // restart
    if (trigger_reset != RESET_UNDEF) {
      if (millis() - reset_timer_start > reset_delay) {
        System.reset(trigger_reset, RESET_NO_WAIT);
      }
      float countdown = ((float) (reset_delay - (millis() - reset_timer_start))) / 1000;
      snprintf(lcd_buffer, sizeof(lcd_buffer), "%.0fs to restart...", countdown);
      lcd->printLineTemp(1, lcd_buffer);
    }

}

/*** logger name capture ***/

void LoggerController::captureName(const char *topic, const char *data) {
  // store name and also assign it to Logger information
  strncpy ( name, data, sizeof(name) );
  name_handler_succeeded = true;
  Serial.println("INFO: logger name '" + String(name) + "'");
  lcd->printLine(1, name);
  if (name_callback) name_callback();
}

/*** state management ***/

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
  if (debug_state) {
    Serial.printf("DEBUG: controller '%s' state saved in memory (if any updates were necessary)\n", version);
  }
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

void LoggerController::resetState() {
  state->version = 0; // force reset of state on restart
  saveState();
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->resetState();
  }
}

/*** command parsing ***/

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
  if (debug_webhooks) {
    Serial.printlnf("DEBUG: webhook debugging is on --> always assemble state log and publish to variable '%s'\n", STATE_LOG_WEBHOOK);
    override_state_log = true;
  }
  if (state->state_logging | override_state_log) {
    assembleStateLog();
    publishStateLog();
  }
  override_state_log = false;

  // state information
  if (command->hasStateChanged()) {
    updateStateVariable();
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
  } else if (parseRestart()) {
    // restart getting parsed
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
    getStateStateLoggingText(state->state_logging, command->data, sizeof(command->data), false, debug_webhooks);
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
    getStateDataLoggingText(state->data_logging, command->data, sizeof(command->data), false, debug_webhooks);
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
      resetState();
      command->success(true);
      getStateStringText(CMD_RESET, CMD_RESET_STATE, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
      command->setLogMsg("restarting system...");
      trigger_reset = RESET_STATE;
      reset_timer_start = millis();
    }
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseRestart() {
  if (command->parseVariable(CMD_RESTART)) {
    command->success(true);
    getInfoValue(command->data, sizeof(command->data), CMD_RESTART);
    command->setLogMsg("restarting system...");
    trigger_reset = RESET_RESTART;
    reset_timer_start = millis();
    
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

/*** state changes ***/

// locking
bool LoggerController::changeLocked(bool on) {
  bool changed = on != state->locked;

  if (changed) {
    state->locked = on;
  }

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: locking Logger") : Serial.println("DEBUG: unlocking Logger");
    else
      on ? Serial.println("DEBUG: Logger already locked") : Serial.println("DEBUG: Logger already unlocked");
  }

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

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: state logging turned on") : Serial.println("DEBUG: state logging turned off");
    else
      on ? Serial.println("DEBUG: state logging already on") : Serial.println("DEBUG: state logging already off");
  }

  if (changed) saveState();

  return(changed);
}

// data log
bool LoggerController::changeDataLogging (bool on) {
  bool changed = on != state->data_logging;

  if (changed) {
    state->data_logging = on;
  }

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: data logging turned on") : Serial.println("DEBUG: data logging turned off");
    else
      on ? Serial.println("DEBUG: data logging already on") : Serial.println("DEBUG: data logging already off");
  }

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

  if (debug_state) {
    if (changed) Serial.printf("DEBUG: setting data logging period to %d %s\n", period, type == LOG_BY_TIME ? "seconds" : "reads");
    else Serial.printf("DEBUG: data logging period unchanged (%d)\n", type == LOG_BY_TIME ? "seconds" : "reads");
  }

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

  if (debug_state) {
    if (changed) Serial.printf("DEBUG: setting data reading period to %d ms\n", period);
    else Serial.printf("DEBUG: data reading period unchanged (%d ms)\n", period);
  }

  if (changed) saveState();

  return(changed);
}

/*** command info to display ***/

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

/*** state info to LCD display ***/

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

void LoggerController::updateDisplayComponentsStateInformation() {
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     (*components_iter)->updateDisplayStateInformation();
  }
}

/*** logger state variable ***/

void LoggerController::updateStateVariable() {
  updateDisplayStateInformation();
  updateDisplayComponentsStateInformation();
  if (state_update_callback) state_update_callback();
  state_variable_buffer[0] = 0; // reset buffer
  assembleStateVariable();
  assembleComponentsStateVariable();
  postStateVariable();
  if (debug_cloud) {
    Serial.printf("DEBUG: updated state variable: %s\n", state_variable);
  }
}

void LoggerController::assembleStateVariable() {
  char pair[60];
  getStateLockedText(state->locked, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  getStateStateLoggingText(state->state_logging, pair, sizeof(pair), false, debug_webhooks); addToStateVariableBuffer(pair);
  getStateDataLoggingText(state->data_logging, pair, sizeof(pair), false, debug_webhooks); addToStateVariableBuffer(pair);
  getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  if (state->data_reader) {
    getStateDataReadingPeriodText(state->data_reading_period, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  }
}

void LoggerController::assembleComponentsStateVariable() {
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     (*components_iter)->assembleStateVariable();
  }
}

void LoggerController::addToStateVariableBuffer(char* info) {
  if (state_variable_buffer[0] == 0) {
    strncpy(state_variable_buffer, info, sizeof(state_variable_buffer));
  } else {
    snprintf(state_variable_buffer, sizeof(state_variable_buffer),
        "%s,%s", state_variable_buffer, info);
  }
}

void LoggerController::postStateVariable() {
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

/*** particle webhook state log ***/

void LoggerController::assembleStartupLog() {
  // include restart details in startup log message
  char msg[50];
  msg[0] = 0;
  if (past_reset == RESET_RESTART) {
    strncpy(msg, "after user-requested restart", sizeof(msg) - 1);
  } else if (past_reset == RESET_STATE) {
    strncpy(msg, "for user-requested state reset", sizeof(msg) - 1);
  } else if (past_reset == RESET_WATCHDOG) {
    strncpy(msg, "triggered by application watchdog", sizeof(msg) - 1);
  }
  msg[sizeof(msg)-1] = 0;
  // id = Logger name, t = state log type, s = state change, m = message, n = notes
  snprintf(state_log, sizeof(state_log), "{\"id\":\"%s\",\"t\":\"startup\",\"s\":[{\"k\":\"startup\",\"v\":\"complete\"}],\"m\":\"%s\",\"n\":\"\"}", name, msg);
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
  if (debug_cloud) {
    Serial.printf("DEBUG: publishing state log %s to event '%s'... ", state_log, STATE_LOG_WEBHOOK);
    if (debug_webhooks) {
      Serial.println();
    }
  }

  if (debug_webhooks) {
    Serial.println("WARNING: state log NOT sent because in WEBHOOKS_DEBUG_ON mode.");
    return(false);
  } else {
    bool success = Particle.connected() && Particle.publish(STATE_LOG_WEBHOOK, state_log, PRIVATE, WITH_ACK);

    if (debug_cloud) {
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    }

    return(success);
  }
}

/*** logger data variable ***/

void LoggerController::updateDataVariable() {
  if (data_update_callback) data_update_callback();
  data_variable_buffer[0] = 0; // reset buffer
  assembleComponentsDataVariable();
  if (Particle.connected()) {
    postDataVariable();
  } else {
    Serial.println("ERROR: particle not (yet) connected.");
  }
  if (debug_cloud) {
    Serial.printf("DEBUG: updated data variable: %s\n", data_variable);
  }
}

void LoggerController::assembleComponentsDataVariable() {
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) 
  {
     (*components_iter)->assembleDataVariable();
  }
}

void LoggerController::addToDataVariableBuffer(char* info) {
  if (data_variable_buffer[0] == 0) {
    strncpy(data_variable_buffer, info, sizeof(data_variable_buffer));
  } else {
    snprintf(data_variable_buffer, sizeof(data_variable_buffer),
        "%s,%s", data_variable_buffer, info);
  }
}

void LoggerController::postDataVariable() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, d = structured data
  snprintf(data_variable, sizeof(data_variable), "{\"dt\":\"%s\",\"d\":[%s]}",
    date_time_buffer, data_variable_buffer);
}

/*** particle webhook data log ***/

bool LoggerController::isTimeForDataLogAndClear() {

  if (state->data_logging_type == LOG_BY_TIME) {
    // go by time
    unsigned long log_period = state->data_logging_period * 1000;
    if ((millis() - last_data_log) > log_period) {
      if (debug_data) {
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.printf("DEBUG: triggering data log at %s (after %d seconds)\n", date_time_buffer, state->data_logging_period);
      }
      return(true);
    }
  } else if (state->data_logging_type == LOG_BY_EVENT) {
    /*
    // not implemented! this needs to be handled by each component individually so requires a mechanism for components to trigger a specific partial data log
    // go by read number
    if (data[0].getN() >= state->data_logging_period) {
      if (debug_data) {
      Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
      Serial.printf("INFO: triggering data log at %s (after %d reads)\n", date_time_buffer, state->data_logging_period);
      }
      return(true);
    }
    */
    Serial.println("ERROR: LOG_BY_EVENT data logging type is not yet implemented!");
  } else {
    Serial.printf("ERROR: unknown logging type stored in state - this should be impossible! %d\n", state->data_logging_type);
  }
  return(false);
}

void LoggerController::clearData(bool clear_persistent) {
  // clear data for components
  for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
     (*components_iter)->clearData(clear_persistent);
  }
}

void LoggerController::logData() {
  // publish data log
  bool override_data_log = false;
  if (debug_webhooks) {
    Serial.printf("DEBUG: webhook debugging is on --> always assemble data log and publish to variable '%s'\n", DATA_LOG_WEBHOOK);
    override_data_log = true;
  }
  if (state->data_logging | override_data_log) {
      // log data for components
      for(components_iter = components.begin(); components_iter != components.end(); components_iter++) {
        (*components_iter)->logData();
      }
  } else {
    if (debug_cloud) {
      Serial.println("DEBUG: data log is turned off --> continue without logging");
    }
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

  if (debug_cloud) {
    if (!state->data_logging) Serial.println("WARNING: publishing data log despite data logging turned off");
    Serial.printf("DEBUG: publishing data log '%s' to event '%s'... ", data_log, DATA_LOG_WEBHOOK);
    if (debug_webhooks) {
      Serial.println();
    }
  }

  if (strlen(data_log) == 0) {
    Serial.println("WARNING: no data log sent because there is none.");
    return(false);
  }

  if (debug_webhooks) {
    Serial.println("WARNING: data log NOT sent because in WEBHOOKS_DEBUG_ON mode.");
    return(false);
  } else {
    bool success = Particle.connected() && Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK);

    if (debug_cloud) {
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    }

    (success) ?
        lcd->printLineTemp(1, "INFO: data log sent") :
        lcd->printLineTemp(1, "ERR: data log error");

    return(success);
  }
}
