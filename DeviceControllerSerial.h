#pragma once
#include "device/DeviceController.h"

// serial data status code
#define SERIAL_DATA_WAITING   0 // waiting for more data
#define SERIAL_DATA_COMPLETE  1 // all data received
#define SERIAL_DATA_ERROR    -1 // encountered error

// controller class
class DeviceControllerSerial : public DeviceController {

  private:

    // serial communication config
    const long serial_baud_rate;
    const long serial_config;
    const int request_wait; // how long to wait after a request is finished to issue the next? [in ms]
    const int error_wait; // how long to wait after an error to re-issue request? [in ms]
    char request_command[10];

  protected:

    // serial communications buffers
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
    unsigned long last_read; // last read
    int serial_data_status = SERIAL_DATA_COMPLETE; // whether data has been received

  public:

    // constructor
    DeviceControllerSerial();
    DeviceControllerSerial (int reset_pin, const long baud_rate, const long serial_config, const char* request_cmd, const int request_wait, const int error_wait) :
      DeviceController(reset_pin), serial_baud_rate(baud_rate), serial_config(serial_config), request_wait(request_wait), error_wait(error_wait) {
        strncpy(request_command, request_cmd, sizeof(request_command) - 1);
        request_command[sizeof(request_command)-1] = 0;
        resetSerialBuffers();
      }

    // setup and loop methods
    virtual void init(); // to be run during setup()
    virtual void update(); // to be run during loop()

    // data updates
    virtual void postDataInformation();

    // serial processing (overwrite virtuals in derived classes)
    virtual bool serialIsActive() { return(true); } // returns whether the serial is active
    virtual bool serialIsManual() { return(false); } // whether the serial port is in manual mode (i.e. should be listening always or make request)
    virtual void startSerialData(); // start processing serial message
    virtual int processSerialData(byte b); // process a byte coming from the serial stream, return a SERIAL_DATA_ return code
    virtual void completeSerialData(); // serial message completely received

    // interact with serial buffer
    void resetSerialBuffers(); // reset all buffers
    void appendToSerialDataBuffer (byte b); // append to total serial data buffer
    void appendToSerialVariableBuffer (byte b);
    void setSerialVariableBuffer (char* var);
    void appendToSerialValueBuffer (byte b);
    void setSerialValueBuffer(char* val);
    void appendToSerialUnitsBuffer (byte b);
    void setSerialUnitsBuffer(char* u);

};

/**** SETUP & LOOP ****/

// init function
void DeviceControllerSerial::init() {
  DeviceController::init();

  // initialize serial communication
  Serial.println("INFO: initializing serial communication, baud rate " + String(serial_baud_rate));
  Serial1.begin(serial_baud_rate, serial_config);

  // empty serial read buffer
  while (Serial1.available()) {
    Serial1.read();
  }
  last_read = millis();
}

// loop function
void DeviceControllerSerial::update() {
  DeviceController::update();

  // check if serial is active
  if (serialIsActive()) {

    // check serial communication
    while (Serial1.available()) {

      // read byte
      byte b = Serial1.read();
      last_read = millis();

      // skip byte if not waiting for response
      if (!waiting_for_response) continue;

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
        Serial.print("INFO: data reading complete at ");
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.println(date_time_buffer);
        completeSerialData();
        waiting_for_response = false;
      } else if (serial_data_status == SERIAL_DATA_ERROR) {
        // error
        Serial.print("WARNING: failed to receive serial data - abort due to unexpected character (byte# " + String(n_byte) + "): ");
        Serial.print(b);
        Serial.print(" = ");
        Serial.println((char) b);
      } else {
        // unexpected behavior!
        Serial.println("ERROR: unexpected return code from processSerialData: " + String(serial_data_status));
      }

    }

    // data request
    bool request_data = false;

    if (serial_data_status == SERIAL_DATA_ERROR && (millis() - last_read) > error_wait) {
      // request data becaus of error timeout
      Serial.print("INFO: error reset timeout reached, re-requesting data");
      request_data = true;
    } else if (serialIsManual() && !waiting_for_response) {
      // request data because in manual mode and not expecting response
      Serial.print("INFO: listening to manual data transmissions");
      request_data = true;
    } else if (!serialIsManual() && (millis() - last_read) > request_wait) {
      // not in manual mode and waited the request_wait time since last activity
      (serial_data_status == SERIAL_DATA_COMPLETE) ?
        Serial.print("INFO: issuing new request for data") :
        Serial.print("INFO: time out, re-issuing request for data");
      request_data = true;
    }

    // (re)-request information
    if (request_data){
      // datetime info
      Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
      Serial.print(" at ");
      Serial.println(date_time_buffer);
      if (!serialIsManual()) Serial1.println(request_command);

      // request parameters
      last_read = millis();
      waiting_for_response = true;
      serial_data_status = SERIAL_DATA_WAITING;
      n_byte = 0;
    }
  }
}

/** DATA UPDATES **/

void DeviceControllerSerial::postDataInformation() {
  snprintf(data_information, sizeof(data_information), "{dt:\"%s\",serial:\"%s\",data:[%s]}",
      date_time_buffer, data_buffer, data_information_buffer);
}

/** SERIAL PROCESSING (virtual functions) **/

void DeviceControllerSerial::startSerialData() {
  unsigned long start_time = millis();
  for (int i=0; i<data.size(); i++) data[i].setNewestDataTime(start_time);
  resetSerialBuffers();
}

int DeviceControllerSerial::processSerialData(byte b) {
  if (b >= 32 && b <= 126) appendToSerialDataBuffer(b); // all data
  return(SERIAL_DATA_WAITING);
}

void DeviceControllerSerial::completeSerialData() {
  updateDataInformation();
  if (serialIsManual()) {
    // log manual entry always right away
    if (getDS()->data_logging) {
      assembleDataLog();
      publishDataLog();
    }
    // reset after each manual entry
    for (int i=0; i<data.size(); i++) data[i].resetValue();
  }
}

/** SERIAL DATA - INTERACT WITH BUFFER **/

void DeviceControllerSerial::resetSerialBuffers() {
  int i;
  for (i=0; i < sizeof(data_buffer); i++) data_buffer[i] = 0;
  for (i=0; i < sizeof(variable_buffer); i++) variable_buffer[i] = 0;
  for (i=0; i < sizeof(value_buffer); i++) value_buffer[i] = 0;
  for (i=0; i < sizeof(units_buffer); i++) units_buffer[i] = 0;
  data_charcounter = 0;
  variable_charcounter = 0;
  value_charcounter = 0;
  units_charcounter = 0;
}

void DeviceControllerSerial::appendToSerialDataBuffer(byte b) {
  data_buffer[data_charcounter] = (char) b;
  data_charcounter++;
}

void DeviceControllerSerial::appendToSerialVariableBuffer(byte b) {
  variable_buffer[variable_charcounter] = (char) b;
  variable_charcounter++;
}

void DeviceControllerSerial::setSerialVariableBuffer(char* var) {
  strncpy(variable_buffer, var, sizeof(variable_buffer) - 1);
  variable_buffer[sizeof(variable_buffer)-1] = 0;
}

void DeviceControllerSerial::appendToSerialValueBuffer(byte b) {
  value_buffer[value_charcounter] = (char) b;
  value_charcounter++;
}

void DeviceControllerSerial::setSerialValueBuffer(char* val) {
  strncpy(value_buffer, val, sizeof(value_buffer) - 1);
  value_buffer[sizeof(value_buffer)-1] = 0;
}

void DeviceControllerSerial::appendToSerialUnitsBuffer(byte b) {
  units_buffer[units_charcounter] = (char) b;
  units_charcounter++;
}

void DeviceControllerSerial::setSerialUnitsBuffer(char* u) {
  strncpy(units_buffer, u, sizeof(units_buffer) - 1);
  units_buffer[sizeof(units_buffer)-1] = 0;
}
