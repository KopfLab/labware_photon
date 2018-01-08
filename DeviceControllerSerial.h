#pragma once
#include "device/DeviceController.h"

// serial communication constants
#define SCALE_DATA_REQUEST  "#" // data request command

// serial data status code
#define SERIAL_DATA_WAITING   0 // waiting for more data
#define SERIAL_DATA_COMPLETE  1 // all data received
#define SERIAL_DATA_ERROR    -1 // encountered error

// controller class
class DeviceControllerSerial : public DeviceController {

  private:

    // serial communication
    const long serial_baud_rate;
    const long serial_config;
    const int request_wait; // how long to wait after a request is finished to issue the next? [in ms]
    const int error_wait; // how long to wait after an error to re-issue request? [in ms]
    char request_command[10];

  protected:

    // serial communications info
    bool waiting_for_response = false; // waiting for response
    int n_byte; // number of bytes received
    long last_read = millis(); // last read
    int serial_data_status = SERIAL_DATA_COMPLETE; // whether data has been received
    char date_time_buffer[21]; // buffer for date time

  public:

    // constructor
    DeviceControllerSerial();
    DeviceControllerSerial (int reset_pin, const long baud_rate, const long serial_config, const char* request_cmd, const int request_wait, const int error_wait) :
      DeviceController(reset_pin), serial_baud_rate(baud_rate), serial_config(serial_config), request_wait(request_wait), error_wait(error_wait) {
        strncpy(request_command, request_cmd, sizeof(request_command) - 1);
        request_command[sizeof(request_command)-1] = 0;
      }

    // setup and loop methods
    virtual void init(); // to be run during setup()
    virtual void update(); // to be run during loop()

    // serial processiong (overwrite in derivec classes)
    virtual bool serialIsActive() { return(true); } // returns whether the serial is active
    virtual bool serialIsManual() { return(false); } // whether the serial port is in manual mode (i.e. should be listening always or make request)
    virtual void startSerialData(); // start processing serial message
    virtual int processSerialData(byte b); // process a byte coming from the serial stream, return a SERIAL_DATA_ return code
    virtual void completeSerialData(); // serial message completely received

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

    if (serial_data_status == SERIAL_DATA_ERROR && millis() - last_read > error_wait) {
      // request data becaus of error timeout
      Serial.print("INFO: error reset timeout reached, re-requesting data");
      request_data = true;
    } else if (serialIsManual() && !waiting_for_response) {
      // request data because in manual mode and not expecting response
      Serial.print("INFO: listening to manual data transmissions");
      request_data = true;
    } else if (!serialIsManual() && millis() - last_read > request_wait) {
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
      if (!serialIsManual()) Serial1.println(SCALE_DATA_REQUEST);

      // request parameters
      last_read = millis();
      waiting_for_response = true;
      serial_data_status = SERIAL_DATA_WAITING;
      n_byte = 0;
    }
  }
}


void DeviceControllerSerial::startSerialData() {

}

int DeviceControllerSerial::processSerialData(byte b) {
  return(SERIAL_DATA_WAITING);
}

void DeviceControllerSerial::completeSerialData() {

  // data callback
  if (data_callback) data_callback();
}
