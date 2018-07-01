// debugging codes (define in main script to enable)
// - CLOUD_DEBUG_ON     // use to enable info messages about cloud variables
// - WEBHOOKS_DEBUG_ON // use to avoid cloud messages from getting sent
// - STATE_DEBUG_ON     // use to enable info messages about state changes
// - DATA_DEBUG_ON      // use to enable info messages about data changes
// - SERIAL_DEBUG_ON    // use to enable info messages about serial data
// - LCD_DEBUG_ON       // see DeviceDisplay.h

#pragma once
#include <vector>
#include "device/DeviceState.h"
#include "device/DeviceInfo.h"
#include "device/DeviceCommands.h"
#include "device/DeviceData.h"
#include "device/DeviceDisplay.h"

// controller class
class DeviceController {

  private:

    // reset PIN
    const int reset_pin;
    bool reset = false;

    // state log exceptions
    bool override_state_log = false;

    // device info
    bool name_handler_registered = false;
    bool name_handler_succeeded = false;
    bool startup_logged = false;

  protected:

    // lcd pointer and buffer
    DeviceDisplay* lcd = 0;
    char lcd_buffer[21];

    // call backs
    void (*name_callback)() = 0;
    void (*command_callback)() = 0;
    void (*data_callback)() = 0;

    // buffer for date time
    char date_time_buffer[25];

    // buffer and information variables
    char state_information[STATE_INFO_MAX_CHAR];
    char state_information_buffer[STATE_INFO_MAX_CHAR-50];
    char data_information[DATA_INFO_MAX_CHAR];
    char data_information_buffer[DATA_INFO_MAX_CHAR-50];

    // buffers for log events
    char state_log[STATE_LOG_MAX_CHAR];
    char data_log[DATA_LOG_MAX_CHAR];
    char data_log_buffer[DATA_LOG_MAX_CHAR-10];

    // data logging (if used in derived class)
    unsigned long last_data_log;

  public:

    // public variables
    char name[20] = "";
    DeviceCommand command;
    std::vector<DeviceData> data;

    // constructor
    DeviceController();
    DeviceController (int reset_pin) : reset_pin(reset_pin) {}
    DeviceController (int reset_pin, DeviceDisplay* lcd) : reset_pin(reset_pin), lcd(lcd) {}

    // setup and loop methods
    virtual void init(); // to be run during setup()
    virtual void update(); // to be run during loop()

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // device name
    void captureName(const char *topic, const char *data);
    void setNameCallback(void (*cb)()); // assign a callback function

    // data information
    virtual bool isTimeForDataLogAndClear() { return(false); } // whether it's time for a data reset and log (if logging is on)
    virtual void clearData(bool all = false); // clear data fields
    virtual void resetData(); // reset data completely
    virtual void assembleDataInformation();
    void addToDataInformation(char* info);
    void setDataCallback(void (*cb)()); // assign a callback function

    // state information
    virtual void assembleStateInformation();
    void addToStateInformation(char* info);

    // state control & persistence functions (implement in derived classes)
    virtual DeviceState* getDS() = 0; // fetch the device state pointer
    virtual void saveDS() = 0; // save device state to EEPROM
    virtual bool restoreDS() = 0; // load device state from EEPROM

    // state change functions (will work in derived classes as long as getDS() is re-implemented)
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);

    // particle command parsing functions
    void setCommandCallback(void (*cb)()); // assign a callback function
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand (); // parse a cloud command
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseReset();

    // command info to LCD display
    virtual void assembleDisplayCommandInformation();
    virtual void showDisplayCommandInformation();

    // state info to LCD display
    virtual void assembleDisplayStateInformation();
    virtual void showDisplayStateInformation();

    // particle variables
    virtual void updateDataInformation();
    virtual void postDataInformation();
    virtual void updateStateInformation();
    virtual void postStateInformation();

    // particle webhook publishing
    virtual void logData();
    virtual void assembleDataLog();
    virtual void assembleDataLog(bool gobal_time_offset);
    void addToDataLog(char* info);
    virtual bool publishDataLog();
    virtual void assembleStartupLog();
    virtual void assembleStateLog();
    virtual bool publishStateLog();
};

/* SETUP & LOOP */

void DeviceController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // lcd
  if (lcd) {
    lcd->init();
    #ifdef DEVICE_VERSION
      lcd->printLine(1, "Starting up (" + String(DEVICE_VERSION) + ")...");
    #else
      lcd->printLine(1, "Starting up...");
    #endif
  }

  //  check for reset
  if(digitalRead(reset_pin) == HIGH) {
    reset = true;
    Serial.println("INFO: reset request detected");
    if (lcd) lcd->printLineTemp(1, "Resetting...");
  }

  // initialize / restore state
  if (!reset){
    Serial.println("INFO: trying to restore state (version " + String(STATE_VERSION) + ") from memory");
    restoreDS();
  } else {
    Serial.println("INFO: resetting state back to default values");
  }

  // startup time info
  Serial.println(Time.format(Time.now(), "INFO: startup time: %Y-%m-%d %H:%M:%S %Z"));

  // state and log variables
  strcpy(state_information, "{}");
  state_information[2] = 0;
  strcpy(data_information, "{}");
  data_information[2] = 0;
  strcpy(state_log, "{}");
  state_log[2] = 0;
  strcpy(data_log, "{}");
  data_log[2] = 0;

  // register particle functions
  Serial.println("INFO: registering device cloud variables");
  Particle.subscribe("spark/", &DeviceController::captureName, this);
  Particle.function(CMD_ROOT, &DeviceController::receiveCommand, this);
  Particle.variable(STATE_INFO_VARIABLE, state_information);
  Particle.variable(DATA_INFO_VARIABLE, data_information);
  #ifdef WEBHOOKS_DEBUG_ON
    // report logs in variables instead of webhooks
    Particle.variable(STATE_LOG_WEBHOOK, state_log);
    Particle.variable(DATA_LOG_WEBHOOK, data_log);
  #endif

  // data log
  last_data_log = 0;
}

void DeviceController::update() {

  // lcd update
  if (lcd) lcd->update();

  // name capture
  if (!name_handler_registered && Particle.connected()){
    name_handler_registered = Particle.publish("spark/device/name");
    if (name_handler_registered) Serial.println("INFO: name handler registered");
  }

  // startup complete
  if (!startup_logged && name_handler_succeeded) {

    // state and data information
    updateStateInformation();
    updateDataInformation();

    if (getDS()->state_logging) {
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
}

/* DEVICE NAME */

void DeviceController::captureName(const char *topic, const char *data) {
  // store name and also assign it to device information
  strncpy ( name, data, sizeof(name) );
  name_handler_succeeded = true;
  Serial.println("INFO: device name '" + String(name) + "'");
  if (lcd) lcd->printLine(1, name);
  if (name_callback) name_callback();
}

void DeviceController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

/* DEVICE STATE CHANGE FUNCTIONS */

// locking
bool DeviceController::changeLocked(bool on) {
  bool changed = on != getDS()->locked;

  if (changed) {
    getDS()->locked = on;
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: locking device") : Serial.println("INFO: unlocking device");
    else
      on ? Serial.println("INFO: device already locked") : Serial.println("INFO: device already unlocked");
  #endif

  if (changed) saveDS();

  return(changed);
}

// state log
bool DeviceController::changeStateLogging (bool on) {
  bool changed = on != getDS()->state_logging;

  if (changed) {
    getDS()->state_logging = on;
    override_state_log = true; // always log this event no matter what
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
    else
      on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
  #endif

  if (changed) saveDS();

  return(changed);
}

// data log
bool DeviceController::changeDataLogging (bool on) {
  bool changed = on != getDS()->data_logging;

  if (changed) {
    getDS()->data_logging = on;
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: data logging turned on") : Serial.println("INFO: data logging turned off");
    else
      on ? Serial.println("INFO: data logging already on") : Serial.println("INFO: data logging already off");
  #endif

  if (changed) saveDS();

  // make sure data is reset
  if (changed && on) resetData();

  return(changed);
}

/* COMMAND PARSING FUNCTIONS */

void DeviceController::setCommandCallback(void (*cb)()) {
  command_callback = cb;
}

bool DeviceController::parseLocked() {
  // decision tree
  if (command.parseVariable(CMD_LOCK)) {
    // locking
    command.extractValue();
    if (command.parseValue(CMD_LOCK_ON)) {
      command.success(changeLocked(true));
    } else if (command.parseValue(CMD_LOCK_OFF)) {
      command.success(changeLocked(false));
    }
    getStateLockedText(getDS()->locked, command.data, sizeof(command.data));
  } else if (getDS()->locked) {
    // device is locked --> no other commands allowed
    command.errorLocked();
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseStateLogging() {
  if (command.parseVariable(CMD_STATE_LOG)) {
    // state logging
    command.extractValue();
    if (command.parseValue(CMD_STATE_LOG_ON)) {
      command.success(changeStateLogging(true));
    } else if (command.parseValue(CMD_STATE_LOG_OFF)) {
      command.success(changeStateLogging(false));
    }
    getStateStateLoggingText(getDS()->state_logging, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseDataLogging() {
  if (command.parseVariable(CMD_DATA_LOG)) {
    // state logging
    command.extractValue();
    if (command.parseValue(CMD_DATA_LOG_ON)) {
      command.success(changeDataLogging(true));
    } else if (command.parseValue(CMD_DATA_LOG_OFF)) {
      command.success(changeDataLogging(false));
    }
    getStateDataLoggingText(getDS()->data_logging, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}

bool DeviceController::parseReset() {
  if (command.parseVariable(CMD_RESET)) {
    command.extractValue();
    if (command.parseValue(CMD_RESET_DATA)) {
      resetData();
      command.success(true);
      getStateStringText(CMD_RESET, CMD_RESET_DATA, command.data, sizeof(command.data), PATTERN_KV_JSON_QUOTED, false);
    }
  }
  return(command.isTypeDefined());
}

/****** WEB COMMAND PROCESSING *******/

int DeviceController::receiveCommand(String command_string) {

  // load, parse and finalize command
  command.load(command_string);
  command.extractVariable();
  parseCommand();
  if (!command.isTypeDefined()) command.errorCommand();

  // lcd info
  assembleDisplayCommandInformation();
  showDisplayCommandInformation();

  // assemble and publish log
  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("INFO: webhook debugging is on --> always assemble state log and publish to variable");
    override_state_log = true;
  #endif
  if (getDS()->state_logging | override_state_log) {
    assembleStateLog();
    publishStateLog();
  }
  override_state_log = false;

  // state information
  if (command.ret_val >= CMD_RET_SUCCESS && command.ret_val != CMD_RET_WARN_NO_CHANGE) {
    updateStateInformation();
  }

  // command reporting callback
  if (command_callback) command_callback();

  // return value
  return(command.ret_val);
}

void DeviceController::parseCommand() {

  // decision tree
  if (parseLocked()) {
    // locked is getting parsed
  } else if (parseStateLogging()) {
    // state logging getting parsed
  } else if (parseDataLogging()) {
    // data logging getting parsed
  } else if (parseReset()) {
    // reset getting parsed
  }

}

/* COMMAND DISPLAY INFORMATION */

void DeviceController::assembleDisplayCommandInformation() {
  if (command.ret_val == CMD_RET_ERR_LOCKED)
    // make user aware of locked status since this may be a confusing error
    snprintf(lcd_buffer, sizeof(lcd_buffer), "LOCK%s: %s", command.type_short, command.command);
  else
    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s: %s", command.type_short, command.command);
}

void DeviceController::showDisplayCommandInformation() {
  if (lcd) lcd->printLineTemp(1, lcd_buffer);
}

/* STATE DISPLAY INFORMATION */

void DeviceController::assembleDisplayStateInformation() {
  uint i = 0;
  if (getDS()->locked) {
    lcd_buffer[i] = 'L';
    i++;
  }
  if (getDS()->state_logging) {
    lcd_buffer[i] = 'S';
    i++;
  }
  if (getDS()->data_logging) {
    lcd_buffer[i] = 'D';
    i++;
  }
  lcd_buffer[i] = 0;
}

void DeviceController::showDisplayStateInformation() {
  if (lcd) {
    lcd->printLine(1, name);
    lcd->printLineRight(1, lcd_buffer, strlen(lcd_buffer) + 1);
  }
}

/* DATA INFORMATION */

void DeviceController::clearData(bool all) {
  #ifdef DATA_DEBUG_ON
    Serial.println(Time.format(Time.now(), "INFO: clearing data at %Y-%m-%d %H:%M:%S %Z"));
  #endif
  for (int i=0; i<data.size(); i++) data[i].clear(all);
}

void DeviceController::resetData() {
  // overwrite for additional reset operations
  clearData(true);
}

void DeviceController::updateDataInformation() {
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

void DeviceController::postDataInformation() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, d = structured data
  snprintf(data_information, sizeof(data_information), "{\"dt\":\"%s\",\"d\":[%s]}",
    date_time_buffer, data_information_buffer);
}

void DeviceController::addToDataInformation(char* info) {
  if (data_information_buffer[0] == 0) {
    strncpy(data_information_buffer, info, sizeof(data_information_buffer));
  } else {
    snprintf(data_information_buffer, sizeof(data_information_buffer),
        "%s,%s", data_information_buffer, info);
  }
}

void DeviceController::assembleDataInformation() {
  for (int i=0; i<data.size(); i++) {
    data[i].assembleInfo();
    addToDataInformation(data[i].json);
  }
}

void DeviceController::setDataCallback(void (*cb)()) {
  data_callback = cb;
}

/* STATE INFORMATION */

void DeviceController::updateStateInformation() {
  #ifdef CLOUD_DEBUG_ON
    Serial.print("INFO: updating state information: ");
  #endif
  lcd_buffer[0] = 0; // reset buffer
  assembleDisplayStateInformation();
  showDisplayStateInformation();
  state_information_buffer[0] = 0; // reset buffer
  assembleStateInformation();
  postStateInformation();
  #ifdef CLOUD_DEBUG_ON
    Serial.println(state_information);
  #endif
}

void DeviceController::postStateInformation() {
  if (Particle.connected()) {
    Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
    // dt = datetime, s = state information
    snprintf(state_information, sizeof(state_information), "{\"dt\":\"%s\",\"s\":[%s]}",
      date_time_buffer, state_information_buffer);
  } else {
    Serial.println("ERROR: particle not (yet) connected.");
  }
}

void DeviceController::addToStateInformation(char* info) {
  if (state_information_buffer[0] == 0) {
    strncpy(state_information_buffer, info, sizeof(state_information_buffer));
  } else {
    snprintf(state_information_buffer, sizeof(state_information_buffer),
        "%s,%s", state_information_buffer, info);
  }
}

void DeviceController::assembleStateInformation() {
  char pair[60];
  getStateLockedText(getDS()->locked, pair, sizeof(pair)); addToStateInformation(pair);
  getStateStateLoggingText(getDS()->state_logging, pair, sizeof(pair)); addToStateInformation(pair);
  getStateDataLoggingText(getDS()->data_logging, pair, sizeof(pair)); addToStateInformation(pair);
}


/***** DATA LOG & WEBHOOK CALLS *******/

void DeviceController::logData() {
  // publish data log
  bool override_data_log = false;
  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("INFO: webhook debugging is on --> always assemble data log and publish to variable");
    override_data_log = true; 
  #endif
  if (getDS()->data_logging | override_data_log) {
      assembleDataLog();
      publishDataLog();
  } else {
    #ifdef CLOUD_DEBUG_ON
      Serial.println("INFO: data log is turned off --> continue without logging");
    #endif
  }
}

void DeviceController::assembleDataLog() { assembleDataLog(true); }

void DeviceController::assembleDataLog(bool global_time_offset) {
  data_log[0] = 0;
  data_log_buffer[0] = 0;
  bool include_time_offset = !global_time_offset;
  for (int i=0; i<data.size(); i++) {
    data[i].assembleLog(include_time_offset);
    addToDataLog(data[i].json);
  }

  // global time
  unsigned long global_time = millis() - (unsigned long) data[0].getDataTime();

  int buffer_size;
  if (global_time_offset) {
    // id = device name, to = time offset (global), d = structured data
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"to\":%lu,\"d\":[%s]}", name, global_time, data_log_buffer);
  } else {
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"d\":[%s]}", name, data_log_buffer);
  }
  if (buffer_size < 0 || buffer_size >= sizeof(data_log)) {
    Serial.println("ERROR: data log buffer not large enough for state log");
    if (lcd) lcd->printLineTemp(1, "ERR: datalog too big");
    // FIXME: implement better size checks!! --> malformatted JSON will crash the webhook
  }
}

void DeviceController::addToDataLog(char* info) {
  if (data_log_buffer[0] == 0) {
    strncpy(data_log_buffer, info, sizeof(data_log_buffer));
  } else {
    snprintf(data_log_buffer, sizeof(data_log_buffer),
        "%s,%s", data_log_buffer, info);
  }
}

bool DeviceController::publishDataLog() {

  #ifdef CLOUD_DEBUG_ON
    if (!getDS()->data_logging) Serial.println("WARNING: publishing data log despite data logging turned off");
    Serial.print("INFO: publishing data log " + String(data_log) + " to event '" + String(DATA_LOG_WEBHOOK) + "'... ");
    #ifdef WEBHOOKS_DEBUG_ON
      Serial.println();
    #endif
  #endif

  #ifdef WEBHOOKS_DEBUG_ON
    Serial.println("WARNING: data log NOT sent because in WEBHOOKS_DEBUG_ON mode.");
  #else
    bool success = Particle.connected() && Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK);

    #ifdef CLOUD_DEBUG_ON
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    #endif

    return(success);
  #endif
}

void DeviceController::assembleStartupLog() {
  // id = device name, t = state log type, s = state change, m = message, n = notes
  snprintf(state_log, sizeof(state_log), "{\"id\":\"%s\",\"t\":\"startup\",\"s\":[{\"k\":\"startup\",\"v\":\"complete\"}],\"m\":\"\",\"n\":\"\"}", name);
}

void DeviceController::assembleStateLog() {
  state_log[0] = 0;
  if (command.data[0] == 0) strcpy(command.data, "{}"); // empty data entry
  // id = device name, t = state log type, s = state change, m = message, n = notes
  int buffer_size = snprintf(state_log, sizeof(state_log),
     "{\"id\":\"%s\",\"t\":\"%s\",\"s\":[%s],\"m\":\"%s\",\"n\":\"%s\"}",
     name, command.type, command.data, command.msg, command.notes);
  if (buffer_size < 0 || buffer_size >= sizeof(state_log)) {
    Serial.println("ERROR: state log buffer not large enough for state log");
    if (lcd) lcd->printLineTemp(1, "ERR: statelog too big");
    // FIXME: implement better size checks!!, i.e. split up call --> malformatted JSON will crash the webhook
  }
}

bool DeviceController::publishStateLog() {
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
