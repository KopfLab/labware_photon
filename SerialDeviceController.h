#pragma once
#include "device/DeviceController.h"
#include "device/SerialDeviceCommands.h"
#include "device/SerialDeviceState.h"

// serial data status code
#define SERIAL_DATA_WAITING   0 // waiting for more data
#define SERIAL_DATA_COMPLETE  1 // all data received
#define SERIAL_DATA_ERROR    -1 // encountered error

// controller class
class SerialDeviceController : public DeviceController {

  private:

    // serial communication config
    const long serial_baud_rate;
    const long serial_config;
    const int error_wait; // how long to wait after an error to re-issue request? [in ms]

    // parameter construction
    void construct(const char* req_cmd);

  protected:

    // serial communications
    char request_command[10];
    char data_buffer[500];
    int data_charcounter;
    char variable_buffer[25];
    int variable_charcounter;
    char value_buffer[25];
    int value_charcounter;
    char units_buffer[20];
    int units_charcounter;

    // serial communications info
    bool waiting_for_response = false; // waiting for response
    int n_byte; // number of bytes received
    unsigned long last_request; // last request (for request timing)
    unsigned long last_byte; // last byte (last byte read - for error timeout)
    int serial_data_status = SERIAL_DATA_COMPLETE; // whether data has been received
    int error_counter; // how many errors/unexpected return in serial transmission (note that a single error will prevent the data from getting logged)

  public:

    // constructor
    SerialDeviceController();

    // without LCD
    SerialDeviceController (int reset_pin, const long baud_rate, const long serial_config, const char* request_cmd, const int error_wait) :
      DeviceController(reset_pin), serial_baud_rate(baud_rate), serial_config(serial_config), error_wait(error_wait) { construct(request_cmd); }

    // with LCD
    SerialDeviceController (int reset_pin, DeviceDisplay* lcd, const long baud_rate, const long serial_config, const char* request_cmd, const int error_wait) :
      DeviceController(reset_pin, lcd), serial_baud_rate(baud_rate), serial_config(serial_config), error_wait(error_wait) { construct(request_cmd); }

    // setup and loop methods
    virtual void init(); // to be run during setup()
    virtual void update(); // to be run during loop()

    // state changes and corresponding commands
    virtual SerialDeviceState* getDSS() = 0; // fetch the serial device state pointer
    bool changeDataReadingPeriod(int period);
    bool parseDataReadingPeriod();
    bool changeDataLoggingPeriod(int period, int type);
    bool parseDataLoggingPeriod();
    virtual void assembleDisplayStateInformation();
    virtual void assembleStateInformation();

    // data updates
    virtual int getNumberDataPoints();
    virtual bool isTimeForDataLogAndClear();
    virtual void postDataInformation();

    // web processing
    virtual void parseCommand (); // parse a cloud command

    // serial processing (overwrite virtuals in derived classes)
    virtual bool serialIsEnabled() { return(true); } // returns whether the serial is enabled, by default always enabled
    virtual bool serialIsManual(); // whether the serial port is in manual mode (i.e. should be listening always or make request)
    virtual void startSerialData(); // start processing serial message
    virtual int processSerialData(byte b); // process a byte coming from the serial stream, return a SERIAL_DATA_ return code
    virtual void completeSerialData(int error_count); // serial message completely received

    // interact with serial buffer
    virtual void sendRequestCommand();
    void resetSerialBuffers(); // reset all buffers
    void resetSerialDataBuffer();
    void resetSerialVariableBuffer();
    void resetSerialValueBuffer();
    void resetSerialUnitsBuffer();

    void appendToSerialDataBuffer (byte b); // append to total serial data buffer
    void appendToSerialVariableBuffer (byte b);
    void appendToSerialValueBuffer (byte b);
    void appendToSerialUnitsBuffer (byte b);

    void setSerialVariableBuffer (char* var);
    void setSerialValueBuffer(char* val);
    void setSerialUnitsBuffer(char* u);

};

/**** CONSTRUCTION ****/

void SerialDeviceController::construct(const char* req_cmd) {
  strncpy(request_command, req_cmd, sizeof(request_command) - 1);
  request_command[sizeof(request_command)-1] = 0;
  resetSerialBuffers();
}

/**** SETUP & LOOP ****/

// init function
void SerialDeviceController::init() {
  DeviceController::init();

  // initialize serial communication
  Serial.println("INFO: initializing serial communication, baud rate " + String(serial_baud_rate));
  Serial1.begin(serial_baud_rate, serial_config);

  // empty serial read buffer
  while (Serial1.available()) {
    Serial1.read();
  }
  last_byte = millis();
  last_request = millis();

  // unlike the base class data log is time based
  last_data_log = millis();
}

// loop function
void SerialDeviceController::update() {
  DeviceController::update();

  // check if serial is active
  if (serialIsEnabled()) {

    // check serial communication
    while (Serial1.available()) {

      // read byte
      byte b = Serial1.read();
      last_byte = millis();

      // skip byte if not waiting for response or already encountered an error
      if (!waiting_for_response || serial_data_status == SERIAL_DATA_ERROR) continue;

      // first byte
      n_byte++;
      if (n_byte == 1) startSerialData();

      // proces byte
      serial_data_status = processSerialData(b);
      waiting_for_response = false;

      // decide how to proceed
      if (serial_data_status == SERIAL_DATA_WAITING) {
        // waiting for more data
        waiting_for_response = true;
      } else if (serial_data_status == SERIAL_DATA_COMPLETE) {
        // message completely received
        #ifdef SERIAL_DEBUG_ON
          Serial.printf("INFO: data reading complete with %d errors at ", error_counter);
          Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
          Serial.println(date_time_buffer);
        #endif
        completeSerialData(error_counter);
        updateDataInformation();
        waiting_for_response = false;
      } else if (serial_data_status == SERIAL_DATA_ERROR) {
        // error
        Serial.printf("WARNING: failed to receive serial data due to unexpected character (byte# %d): %x = %x\n", n_byte, b, (char) b);
        if (lcd) lcd->printLineTemp(1, "ERR: serial error");
        error_counter++;
      } else {
        // unexpected behavior!
        Serial.println("ERROR: unexpected return code from processSerialData: " + String(serial_data_status));
      }

    }

    // data request
    bool request_data = false;

    if (serial_data_status == SERIAL_DATA_ERROR && (millis() - last_byte) > error_wait) {
      // request data becaus of error timeout
      #ifdef SERIAL_DEBUG_ON
        Serial.print("INFO: error reset timeout reached, re-requesting data");
      #endif
      request_data = true;
    } else if (serialIsManual() && !waiting_for_response) {
      // "request"/listen to data because in manual mode and not waiting for response yet
      #ifdef SERIAL_DEBUG_ON
        Serial.print("INFO: listening to manual data transmissions");
      #endif
      request_data = true;
    } else if (!serialIsManual() && (millis() - last_request) > getDSS()->data_reading_period) {
      // not in manual mode and waited the read period time since last activity
      #ifdef SERIAL_DEBUG_ON
        (serial_data_status == SERIAL_DATA_COMPLETE) ?
          Serial.print("INFO: issuing new request for data") :
          Serial.print("INFO: time out, re-issuing request for data");
      #endif
      if (serial_data_status != SERIAL_DATA_COMPLETE && lcd) lcd->printLineTemp(1, "ERR: serial timeout");
      request_data = true;
    }

    // (re)-request information
    if (request_data){
      // datetime info
      #ifdef SERIAL_DEBUG_ON
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.printf(" at %s\n", date_time_buffer);
      #endif
      if (!serialIsManual()) sendRequestCommand();

      // request parameters
      last_byte = millis();
      last_request = millis();
      waiting_for_response = true;
      serial_data_status = SERIAL_DATA_WAITING;
      error_counter = 0;
      n_byte = 0;
    }
  }
}

/**** CHANGING STATE ****/

// reading period
bool SerialDeviceController::changeDataReadingPeriod(int period) {
  bool changed = period != getDSS()->data_reading_period;

  if (changed) {
    getDSS()->data_reading_period = period;
  }

  #ifdef STATE_DEBUG_ON
    if (changed) Serial.printf("INFO: setting data reading period to %d ms\n", period);
    else Serial.printf("INFO: data reading period unchanged (%d ms)\n", period);
  #endif

  if (changed) saveDS();

  return(changed);
}

bool SerialDeviceController::parseDataReadingPeriod() {
  if (command.parseVariable(CMD_DATA_READ_PERIOD)) {
    // parse read period
    command.extractValue();
    if (command.parseValue(CMD_DATA_READ_PERIOD_MANUAL)){
      // manual reads
      command.success(changeDataReadingPeriod(READ_MANUAL));
    } else {
      int read_period = atoi(command.value);
      if (read_period > 0) {
        command.extractUnits();
        if (command.parseUnits(CMD_DATA_READ_PERIOD_MS)) {
          // milli seconds (the base unit)
          read_period = read_period;
        } else if (command.parseUnits(CMD_DATA_READ_PERIOD_SEC)) {
          // seconds
          read_period = 1000 * read_period;
        } else if (command.parseUnits(CMD_DATA_READ_PERIOD_MIN)) {
          // minutes
          read_period = 1000 * 60 * read_period;
        } else {
          // unrecognized units
          command.errorUnits();
        }
        // assign read period
        if (!command.isTypeDefined()) {
          if (read_period < getDSS()->data_reading_period_min)
            // make sure bigger than minimum
            command.error(CMD_RET_ERR_READ_LARGER_MIN, CMD_RET_ERR_READ_LARGER_MIN_TEXT);
          else if (getDSS()->data_logging_type == LOG_BY_TIME && getDSS()->data_logging_period * 1000 <= read_period)
            // make sure smaller than log period
            command.error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
          else
            command.success(changeDataReadingPeriod(read_period));
        }
      } else {
        // invalid value
        command.errorValue();
      }
    }
    getStateDataReadingPeriodText(getDSS()->data_reading_period, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}

// logging period
bool SerialDeviceController::changeDataLoggingPeriod(int period, int type) {
  bool changed = period != getDSS()->data_logging_period | type != getDSS()->data_logging_type;

  if (changed) {
    getDSS()->data_logging_period = period;
    getDSS()->data_logging_type = type;
  }

  #ifdef STATE_DEBUG_ON
    if (changed) Serial.printf("INFO: setting data logging period to %d %s\n", period, type == LOG_BY_TIME ? "seconds" : "reads");
    else Serial.printf("INFO: data logging period unchanged (%d)\n", type == LOG_BY_TIME ? "seconds" : "reads");
  #endif

  if (changed) saveDS();

  return(changed);
}

bool SerialDeviceController::parseDataLoggingPeriod() {
  if (command.parseVariable(CMD_DATA_LOG_PERIOD)) {
    // parse read period
    command.extractValue();
    int log_period = atoi(command.value);
    if (log_period > 0) {
      command.extractUnits();
      uint8_t log_type = LOG_BY_TIME;
      if (command.parseUnits(CMD_DATA_LOG_PERIOD_NUMBER)) {
        // events
        log_type = LOG_BY_READ;
      } else if (command.parseUnits(CMD_DATA_LOG_PERIOD_SEC)) {
        // seconds (the base unit)
        log_period = log_period;
      } else if (command.parseUnits(CMD_DATA_LOG_PERIOD_MIN)) {
        // minutes
        log_period = 60 * log_period;
      } else if (command.parseUnits(CMD_DATA_LOG_PERIOD_HR)) {
        // minutes
        log_period = 60 * 60 * log_period;
      } else {
        // unrecognized units
        command.errorUnits();
      }
      // assign read period
      if (!command.isTypeDefined()) {
        if (log_type == LOG_BY_TIME && log_period * 1000 <= getDSS()->data_reading_period)
          command.error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
        else
          command.success(changeDataLoggingPeriod(log_period, log_type));
      }
    } else {
      // invalid value
      command.errorValue();
    }
    getStateDataLoggingPeriodText(getDSS()->data_logging_period, getDSS()->data_logging_type, command.data, sizeof(command.data));
  }
  return(command.isTypeDefined());
}

/****** STATE INFORMATION *******/

void SerialDeviceController::assembleDisplayStateInformation() {
  DeviceController::assembleDisplayStateInformation();
  uint i = strlen(lcd_buffer);

  // details on data logging
  getStateDataLoggingPeriodText(getDSS()->data_logging_period, getDSS()->data_logging_type,
      lcd_buffer + i, sizeof(lcd_buffer) - i, true);
  i = strlen(lcd_buffer);

  // data reading period
  lcd_buffer[i] = 'R'; i++;
  if (serialIsManual()) {
    lcd_buffer[i] = 'M';
    lcd_buffer[i+1] = 0;
  } else {
    getStateDataReadingPeriodText(getDSS()->data_reading_period, lcd_buffer + i, sizeof(lcd_buffer) - i, true);
  }
}

void SerialDeviceController::assembleStateInformation() {
  DeviceController::assembleStateInformation();
  char pair[60];
  getStateDataReadingPeriodText(getDSS()->data_reading_period, pair, sizeof(pair)); addToStateInformation(pair);
  getStateDataLoggingPeriodText(getDSS()->data_logging_period, getDSS()->data_logging_type, pair, sizeof(pair)); addToStateInformation(pair);
}

/**** DATA LOGGING ****/

int SerialDeviceController::getNumberDataPoints() {
  // default is that the first data type is representative
  return(data[0].getN());
}

bool SerialDeviceController::isTimeForDataLogAndClear() {

  if (!serialIsEnabled()) return(false);

  if (getDSS()->data_logging_type == LOG_BY_TIME) {
    // go by time
    unsigned long log_period = getDSS()->data_logging_period * 1000;
    if ((millis() - last_data_log) > log_period) {
      #ifdef DATA_DEBUG_ON
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.printf("INFO: triggering data log at %s (after %d seconds)\n", date_time_buffer, getDSS()->data_logging_period);
      #endif
      return(true);
    }
  } else if (getDSS()->data_logging_type == LOG_BY_READ) {
    // go by read number
    if (getNumberDataPoints() >= getDSS()->data_logging_period) {
      #ifdef DATA_DEBUG_ON
      Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
      Serial.printf("INFO: triggering data log at %s (after %d reads)\n", date_time_buffer, getDSS()->data_logging_period);
      #endif
      return(true);
    }
  } else {
    Serial.printf("ERROR: unknown logging type stored in state - this should be impossible! %d\n", getDSS()->data_logging_type);
  }
  return(false);
}

/** DATA UPDATES **/

void SerialDeviceController::postDataInformation() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, r = raw serial, e = number of errors encountered in raw serial, d = parsed data
  snprintf(data_information, sizeof(data_information), "{\"dt\":\"%s\",\"r\":\"%s\",\"e\":%d,\"d\":[%s]}",
      date_time_buffer, data_buffer, error_counter, data_information_buffer);
}

/** WEB COMMAND PROCESSING **/

void SerialDeviceController::parseCommand() {

  DeviceController::parseCommand();

  if (command.isTypeDefined()) {
    // command processed successfully by parent function
  } else if (parseDataReadingPeriod()) {
    // parsing reading period
  } else if (parseDataLoggingPeriod()) {
    // parsing logging period
  }

}

/** SERIAL PROCESSING **/

void SerialDeviceController::sendRequestCommand() {
  Serial1.println(request_command);
}

void SerialDeviceController::startSerialData() {
  unsigned long start_time = millis();
  for (int i=0; i<data.size(); i++) data[i].setNewestDataTime(start_time);
  resetSerialBuffers();
}

int SerialDeviceController::processSerialData(byte b) {
  if (b >= 32 && b <= 126) appendToSerialDataBuffer(b); // all data
  return(SERIAL_DATA_WAITING);
}

void SerialDeviceController::completeSerialData(int error_count) {
  // implement in derived classes, typically only save values if error count == 0
}

bool SerialDeviceController::serialIsManual(){
  return(getDSS()->data_reading_period == 0);
}

/** SERIAL DATA - INTERACT WITH BUFFER **/

void SerialDeviceController::resetSerialBuffers() {
  resetSerialDataBuffer();
  resetSerialVariableBuffer();
  resetSerialValueBuffer();
  resetSerialUnitsBuffer();
}

void SerialDeviceController::resetSerialDataBuffer() {
  for (int i=0; i < sizeof(data_buffer); i++) data_buffer[i] = 0;
  data_charcounter = 0;
}

void SerialDeviceController::resetSerialVariableBuffer() {
  for (int i=0; i < sizeof(variable_buffer); i++) variable_buffer[i] = 0;
  variable_charcounter = 0;
}

void SerialDeviceController::resetSerialValueBuffer() {
  for (int i=0; i < sizeof(value_buffer); i++) value_buffer[i] = 0;
  value_charcounter = 0;
}

void SerialDeviceController::resetSerialUnitsBuffer() {
  for (int i=0; i < sizeof(units_buffer); i++) units_buffer[i] = 0;
  units_charcounter = 0;
}

void SerialDeviceController::appendToSerialDataBuffer(byte b) {
  if (data_charcounter < sizeof(data_buffer) - 2) {
    data_buffer[data_charcounter] = (char) b;
    data_charcounter++;
  } else {
    Serial.println("ERROR: serial data buffer not big enough");
  }
}

void SerialDeviceController::appendToSerialVariableBuffer(byte b) {
  if (variable_charcounter < sizeof(variable_buffer) - 2) {
    variable_buffer[variable_charcounter] = (char) b;
    variable_charcounter++;
  } else {
    Serial.println("ERROR: serial variable buffer not big enough");
  }
}

void SerialDeviceController::setSerialVariableBuffer(char* var) {
  resetSerialVariableBuffer();
  strncpy(variable_buffer, var, sizeof(variable_buffer) - 1);
  variable_buffer[sizeof(variable_buffer)-1] = 0;
  variable_charcounter = strlen(variable_buffer);
}

void SerialDeviceController::appendToSerialValueBuffer(byte b) {
  if (value_charcounter < sizeof(value_buffer) - 2) {
    value_buffer[value_charcounter] = (char) b;
    value_charcounter++;
  } else {
    Serial.println("ERROR: serial value buffer not big enough");
  }
}

void SerialDeviceController::setSerialValueBuffer(char* val) {
  resetSerialValueBuffer();
  strncpy(value_buffer, val, sizeof(value_buffer) - 1);
  value_buffer[sizeof(value_buffer)-1] = 0;
  value_charcounter = strlen(value_buffer);
}

void SerialDeviceController::appendToSerialUnitsBuffer(byte b) {
  if (units_charcounter < sizeof(units_buffer) - 2) {
    units_buffer[units_charcounter] = (char) b;
    units_charcounter++;
  } else {
    Serial.println("ERROR: serial units buffer not big enough");
  }
}

void SerialDeviceController::setSerialUnitsBuffer(char* u) {
  resetSerialUnitsBuffer();
  strncpy(units_buffer, u, sizeof(units_buffer) - 1);
  units_buffer[sizeof(units_buffer)-1] = 0;
  units_charcounter = strlen(units_buffer);
}
