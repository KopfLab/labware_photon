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
    bool overwrite_state_log = false;

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
    char date_time_buffer[21];

    // buffer for information variables
    char state_information_buffer[STATE_INFO_MAX_CHAR-50];
    char data_information_buffer[DATA_INFO_MAX_CHAR-50];

    // buffers for log events
    char state_log[STATE_LOG_MAX_CHAR];
    char data_log[DATA_LOG_MAX_CHAR];

  public:

    // command
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
    char name[20];
    void captureName(const char *topic, const char *data);
    void setNameCallback(void (*cb)()); // assign a callback function

    // data information
    char data_information[DATA_INFO_MAX_CHAR];
    virtual void assembleDataInformation();
    void addToDataInformation(char* info, char* sep);
    void setDataCallback(void (*cb)()); // assign a callback function

    // state information
    char state_information[STATE_INFO_MAX_CHAR];
    virtual void assembleStateInformation();
    void addToStateInformation(char* info, char* sep);

    // state control & persistence functions (implement in derived classes)
    virtual DeviceState* getDS() = 0; // fetch the device state pointer
    virtual void saveDS() = 0; // save device state to EEPROM
    virtual bool restoreDS() = 0; // load device state from EEPROM

    // state change functions (will work in derived classes as long as getDS() is re-implemented)
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);
    bool changeTimezone(int tz);

    // particle command parsing functions
    void setCommandCallback(void (*cb)()); // assign a callback function
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand () = 0; // parse a cloud command
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseTimezone();

    // particle variables
    virtual void updateDataInformation();
    virtual void postDataInformation();
    virtual void updateStateInformation();
    virtual void postStateInformation();

    // particle webhook publishing
    virtual void assembleDataLog();
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
  Time.zone(getDS()->timezone);
  time_t time = Time.now();
  Serial.println(Time.format(time, "INFO: startup time: %Y-%m-%d %H:%M:%S"));

  // state information
  updateStateInformation();
  updateDataInformation();

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
    Serial.println("INFO: name handler registered");
  }

  // startup log
  if (!startup_logged && name_handler_succeeded) {
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
}

/* DEVICE NAME */

void DeviceController::captureName(const char *topic, const char *data) {
  // store name and also assign it to device information
  strncpy ( name, data, sizeof(name) );
  name_handler_succeeded = true;
  Serial.println("INFO: device name " + String(name));
  if (name_callback) name_callback();
}

void DeviceController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

/* DATA INFORMATION */

void DeviceController::updateDataInformation() {
  Serial.print("INFO: updating data information: ");
  data_information_buffer[0] = 0; // reset buffer
  assembleDataInformation();
  postDataInformation();
  Serial.println(data_information);
  if (data_callback) data_callback();
}

void DeviceController::postDataInformation() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  snprintf(data_information, sizeof(data_information), "{dt:\"%s\",data:[%s]}",
    date_time_buffer, data_information_buffer);
}

void DeviceController::addToDataInformation(char* info, char* sep = ",") {
  if (data_information_buffer[0] == 0) {
    strncpy(data_information_buffer, info, sizeof(data_information_buffer));
  } else {
    snprintf(data_information_buffer, sizeof(data_information_buffer),
        "%s%s%s", data_information_buffer, sep, info);
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
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  snprintf(state_information, sizeof(state_information), "{dt:\"%s\",state:[%s]}",
    date_time_buffer, state_information_buffer);
}

void DeviceController::addToStateInformation(char* info, char* sep = ",") {
  if (state_information_buffer[0] == 0) {
    strncpy(state_information_buffer, info, sizeof(state_information_buffer));
  } else {
    snprintf(state_information_buffer, sizeof(state_information_buffer),
        "%s%s%s", state_information_buffer, sep, info);
  }
}

void DeviceController::assembleStateInformation() {
  char pair[60];
  getStateTimezoneText(getDS()->timezone, pair, sizeof(pair)); addToStateInformation(pair);
  getStateLockedText(getDS()->locked, pair, sizeof(pair)); addToStateInformation(pair);
  getStateStateLoggingText(getDS()->state_logging, pair, sizeof(pair)); addToStateInformation(pair);
  getStateDataLoggingText(getDS()->data_logging, pair, sizeof(pair)); addToStateInformation(pair);
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
    overwrite_state_log = true; // always log this event no matter what
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
  return(changed);
}

// time zone
bool DeviceController::changeTimezone (int tz) {
  bool changed = tz != getDS()->timezone;
  if (changed) {
    getDS()->timezone = tz;
    Serial.print("INFO: timezone changed to " + String(getDS()->timezone));
    Time.zone(getDS()->timezone);
    time_t time = Time.now();
    Serial.println(Time.format(time, ", time: %Y-%m-%d %H:%M:%S"));
    saveDS();
  } else {
    Serial.println("INFO: timezone unchanged (" + String(getDS()->timezone) + ")");
  }
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

bool DeviceController::parseTimezone() {
  if (command.parseVariable(CMD_TIMEZONE)) {
    // timezone
    command.extractValue();
    int tz = atoi(command.value);
    (tz >= -12 && tz <= 14) ? command.success(changeTimezone(tz)) : command.errorValue();
    getStateTimezoneText(getDS()->timezone, command.data, sizeof(command.data));
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
  if (getDS()->state_logging | overwrite_state_log) {
    assembleStateLog();
    publishStateLog();
  }
  overwrite_state_log = false;

  // state information
  if (command.ret_val >= CMD_RET_SUCCESS && command.ret_val != CMD_RET_WARN_NO_CHANGE) {
    updateStateInformation();
  }

  // command reporting callback
  if (command_callback) command_callback();

  // return value
  return(command.ret_val);
}

/***** WEBHOOK CALLS *******/

void DeviceController::assembleDataLog() {

}

bool DeviceController::publishDataLog() {
  Serial.print("INFO: publishing log " + String(data_log) + " to event '" + String(DATA_LOG_WEBHOOK) + "'... ");
  if(Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK)) {
    Serial.println("successful.");
    return(true);
  } else {
    Serial.println("failed!");
    return(false);
  }
}

void DeviceController::assembleStateLog() {
  if (command.data[0] == 0) {
    // add empty data entry
    strcpy(command.data, "{}");
  }
  snprintf(state_log, sizeof(state_log),
     "{\"type\":\"%s\",\"data\":[%s],\"msg\":\"%s\",\"notes\":\"%s\"}",
     command.type, command.data, command.msg, command.notes);
}

bool DeviceController::publishStateLog() {
  Serial.print("INFO: publishing log " + String(state_log) + " to event '" + String(STATE_LOG_WEBHOOK) + "'... ");
  if(Particle.publish(STATE_LOG_WEBHOOK, state_log, PRIVATE, WITH_ACK)) {
    Serial.println("successful.");
    return(true);
  } else {
    Serial.println("failed!");
    return(false);
  }
}
