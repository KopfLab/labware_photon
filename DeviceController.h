#pragma once
#include <vector>
#include "device/DeviceState.h"
#include "device/DeviceInfo.h"
#include "device/DeviceCommands.h"
#include "device/DeviceData.h"

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

    // call backs
    void (*name_callback)();
    void (*command_callback)();
    void (*data_callback)();

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

  public:

    // public variables
    char name[20];
    DeviceCommand command;
    std::vector<DeviceData> data;

    // constructor
    DeviceController();
    DeviceController (int reset_pin) : reset_pin(reset_pin) {}

    // setup and loop methods
    virtual void init(); // to be run during setup()
    virtual void update(); // to be run during loop()

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // device name
    void captureName(const char *topic, const char *data);
    void setNameCallback(void (*cb)()); // assign a callback function

    // data information
    virtual bool isTimeForDataReset() { return(false); } // whether it's time for a data reset and log (if logging is on)
    void resetData(); // reset all data fields
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
    virtual void parseCommand () = 0; // parse a cloud command
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();

    // particle variables
    virtual void updateDataInformation();
    virtual void postDataInformation();
    virtual void updateStateInformation();
    virtual void postStateInformation();

    // particle webhook publishing
    virtual void assembleDataLog();
    virtual void assembleDataLog(bool gobal_time_offset);
    void addToDataLog(char* info);
    virtual bool publishDataLog();
    virtual void assembleStateLog();
    virtual bool publishStateLog();
};

/* SETUP & LOOP */

void DeviceController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  //  check for reset
  if(digitalRead(reset_pin) == HIGH) {
    reset = true;
    Serial.println("INFO: reset request detected");
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

  // register particle functions
  Serial.println("INFO: registering device cloud variables");
  Particle.subscribe("spark/", &DeviceController::captureName, this);
  Particle.function(CMD_ROOT, &DeviceController::receiveCommand, this);
  Particle.variable(STATE_INFO_VARIABLE, state_information);
  Particle.variable(DATA_INFO_VARIABLE, data_information);
}

void DeviceController::update() {

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
      command.makeStartupLog();
      assembleStateLog();
      publishStateLog();
    } else {
      Serial.println("INFO: start-up completed (not logged).");
    }
    startup_logged = true;
  }

  // data reset
  if (isTimeForDataReset()) {
    // publish data log before reset
    if (getDS()->data_logging) {
        assembleDataLog();
        publishDataLog();
    }
    // resetting data
    resetData();
  }
}

/* DEVICE NAME */

void DeviceController::captureName(const char *topic, const char *data) {
  // store name and also assign it to device information
  strncpy ( name, data, sizeof(name) );
  name_handler_succeeded = true;
  Serial.println("INFO: device name '" + String(name) + "'");
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
    on ? Serial.println("INFO: locking device") : Serial.println("INFO: unlocking device");
    saveDS();
  } else {
    on ? Serial.println("INFO: device already locked") : Serial.println("INFO: device already unlocked");
  }
  return(changed);
}

// state log
bool DeviceController::changeStateLogging (bool on) {
  bool changed = on != getDS()->state_logging;

  if (changed) {
    getDS()->state_logging = on;
    on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
    override_state_log = true; // always log this event no matter what
    saveDS();
  } else {
    on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
  }
  return(changed);
}

// data log
bool DeviceController::changeDataLogging (bool on) {
  bool changed = on != getDS()->data_logging;

  if (changed) {
    getDS()->data_logging = on;
    on ? Serial.println("INFO: data logging turned on") : Serial.println("INFO: data logging turned off");
    saveDS();
  } else {
    on ? Serial.println("INFO: data logging already on") : Serial.println("INFO: data logging already off");
  }

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

/****** WEB COMMAND PROCESSING *******/

int DeviceController::receiveCommand(String command_string) {

  // load, parse and finalize command
  command.load(command_string);
  command.extractVariable();
  parseCommand();
  if (!command.isTypeDefined()) command.errorCommand();

  // assemble and publish log
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


/* DATA INFORMATION */

void DeviceController::resetData() {
  Serial.println(Time.format(Time.now(), "INFO: resetting data at %Y-%m-%d %H:%M:%S %Z"));
  for (int i=0; i<data.size(); i++) data[i].resetValue();
}

void DeviceController::updateDataInformation() {
  Serial.print("INFO: updating data information: ");
  data_information_buffer[0] = 0; // reset buffer
  assembleDataInformation();
  postDataInformation();
  Serial.println(data_information);
  if (data_callback) data_callback();
}

void DeviceController::postDataInformation() {
  if (Particle.connected()) {
    Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
    snprintf(data_information, sizeof(data_information), "{dt:\"%s\",data:[%s]}",
      date_time_buffer, data_information_buffer);
  } else {
    Serial.println("ERROR: particle not (yet) connected.");
  }
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
  Serial.print("INFO: updating state information: ");
  state_information_buffer[0] = 0; // reset buffer
  assembleStateInformation();
  postStateInformation();
  Serial.println(state_information);
}

void DeviceController::postStateInformation() {
  if (Particle.connected()) {
    Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
    snprintf(state_information, sizeof(state_information), "{dt:\"%s\",state:[%s]}",
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


/***** WEBHOOK CALLS *******/

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
  unsigned long global_time = millis() - data[0].data_time;
  int buffer_size;
  if (global_time_offset) {
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"to\":%lu,\"data\":[%s]}", global_time, data_log_buffer);
  } else {
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"data\":[%s]}", data_log_buffer);
  }
  if (buffer_size < 0 || buffer_size >= sizeof(data_log)) {
    Serial.println("ERROR: data log buffer not large enough for state log");
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
  if (!getDS()->data_logging) Serial.println("WARNING: publishing data log despite data logging turned off");
  Serial.print("INFO: publishing data log " + String(data_log) + " to event '" + String(DATA_LOG_WEBHOOK) + "'... ");
  if(Particle.connected() && Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK)) {
    Serial.println("successful.");
    return(true);
  } else {
    Serial.println("failed!");
    return(false);
  }
}

void DeviceController::assembleStateLog() {
  state_log[0] = 0;
  if (command.data[0] == 0) strcpy(command.data, "{}"); // empty data entry
  int buffer_size = snprintf(state_log, sizeof(state_log),
     "{\"type\":\"%s\",\"data\":[%s],\"msg\":\"%s\",\"notes\":\"%s\"}",
     command.type, command.data, command.msg, command.notes);
  if (buffer_size < 0 || buffer_size >= sizeof(state_log)) {
    Serial.println("ERROR: state log buffer not large enough for state log");
    // FIXME: implement better size checks!! --> malformatted JSON will crash the webhook
  }
}

bool DeviceController::publishStateLog() {
  Serial.print("INFO: publishing state log " + String(state_log) + " to event '" + String(STATE_LOG_WEBHOOK) + "'... ");
  if(Particle.connected() && Particle.publish(STATE_LOG_WEBHOOK, state_log, PRIVATE, WITH_ACK)) {
    Serial.println("successful.");
    return(true);
  } else {
    Serial.println("failed!");
    return(false);
  }
}
