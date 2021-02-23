#include "application.h"
#include "SerialReaderLoggerComponent.h"

/*** setup ***/

void SerialReaderLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // initialize serial communication
    Serial.printlnf("INFO: initializing serial communication, baud rate '%ld'", serial_baud_rate);
    Serial1.begin(serial_baud_rate, serial_config);

    // empty serial read buffer
    while (Serial1.available()) Serial1.read();
}

/*** read data ***/

void SerialReaderLoggerComponent::sendSerialDataRequest() {
    if (ctrl->debug_data) {
        Serial.printlnf("DEBUG: sending command '%s' over serial connection for component '%s'", request_command, id);
    }
    Serial1.println(*request_command);
}

void SerialReaderLoggerComponent::idleDataRead() {
    // discard everyhing coming from the serial connection
    while (Serial1.available()) Serial1.read();
}

void SerialReaderLoggerComponent::initiateDataRead() {
    // initiate data read by sending command and registering resetting number of received bytes
    DataReaderLoggerComponent::initiateDataRead();
    if (!isManualDataReader()) sendSerialDataRequest();
    n_byte = 0;
}

void SerialReaderLoggerComponent::readData() {
    // check serial connection for data
    while (data_read_status == DATA_READ_WAITING && Serial1.available()) {

        // read byte
        new_byte = Serial1.read();
        n_byte++;

        // first byte
        if (n_byte == 1) startData();

        // proces byte
        processNewByte();

        // if working with a data pattern --> mark completion
        if (data_pattern_pos >= data_pattern_size) {
            data_read_status = DATA_READ_COMPLETE;
        }

    }
}

void SerialReaderLoggerComponent::completeDataRead() {
    DataReaderLoggerComponent::completeDataRead();
}

void SerialReaderLoggerComponent::registerDataReadError() {
    Serial.printlnf("WARNING: registering data read error at byte# %d: %x = %x", n_byte, new_byte, (char) new_byte);
    ctrl->lcd->printLineTemp(1, "ERR: serial error");
    error_counter++;
}

void SerialReaderLoggerComponent::handleDataReadTimeout() {
    DataReaderLoggerComponent::handleDataReadTimeout();
    if (ctrl->debug_data) {
        Serial.printlnf("DEBUG: serial data buffer = '%s'", data_buffer);
    }
}

/*** manage data ***/

void SerialReaderLoggerComponent::startData() {
  DataReaderLoggerComponent::startData();
  resetSerialBuffers();
  data_pattern_pos = 0;
}

void SerialReaderLoggerComponent::processNewByte() {
  if (debug_component) {
    (new_byte >= 32 && new_byte <= 126) ?
      Serial.printlnf("SERIAL: byte# %03d: %i (dec) = %x (hex) = '%c' (char)", n_byte, (int) new_byte, new_byte, (char) new_byte) :
      Serial.printlnf("SERIAL: byte# %03d: %i (dec) = %x (hex) = (special char)", n_byte, (int) new_byte, new_byte);
  }
  if (new_byte >= 32 && new_byte <= 126) {
    appendToSerialDataBuffer(new_byte); // all data
  } else if (new_byte == 13 || new_byte == 10) {
    // 13 = carriage return, 10 = line feed
    appendToSerialDataBuffer(10); // add new line to all data
  }
  // extend in derived classes
}

void SerialReaderLoggerComponent::finishData() {
    // extend in derived classes, typically only save values if error_count == 0
}

/*** interact with serial data buffers ***/

void SerialReaderLoggerComponent::resetSerialBuffers() {
  resetSerialDataBuffer();
  resetSerialVariableBuffer();
  resetSerialValueBuffer();
  resetSerialUnitsBuffer();
}

void SerialReaderLoggerComponent::resetSerialDataBuffer() {
  for (int i=0; i < sizeof(data_buffer); i++) data_buffer[i] = 0;
  data_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialVariableBuffer() {
  for (int i=0; i < sizeof(variable_buffer); i++) variable_buffer[i] = 0;
  variable_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialValueBuffer() {
  for (int i=0; i < sizeof(value_buffer); i++) value_buffer[i] = 0;
  value_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialUnitsBuffer() {
  for (int i=0; i < sizeof(units_buffer); i++) units_buffer[i] = 0;
  units_charcounter = 0;
}

void SerialReaderLoggerComponent::appendToSerialDataBuffer(byte b) {
  if (data_charcounter < sizeof(data_buffer) - 2) {
    data_buffer[data_charcounter] = (char) b;
    data_charcounter++;
  } else {
    Serial.println("ERROR: serial data buffer not big enough");
    registerDataReadError();
    data_read_status = DATA_READ_IDLE;
  }
}

void SerialReaderLoggerComponent::appendToSerialVariableBuffer(byte b) {
  if (variable_charcounter < sizeof(variable_buffer) - 2) {
    variable_buffer[variable_charcounter] = (char) b;
    variable_charcounter++;
  } else {
    Serial.println("ERROR: serial variable buffer not big enough");
    registerDataReadError();
    data_read_status = DATA_READ_IDLE;
  }
}

void SerialReaderLoggerComponent::setSerialVariableBuffer(char* var) {
  resetSerialVariableBuffer();
  strncpy(variable_buffer, var, sizeof(variable_buffer) - 1);
  variable_buffer[sizeof(variable_buffer)-1] = 0;
  variable_charcounter = strlen(variable_buffer);
}

void SerialReaderLoggerComponent::appendToSerialValueBuffer(byte b) {
  if (value_charcounter < sizeof(value_buffer) - 2) {
    value_buffer[value_charcounter] = (char) b;
    value_charcounter++;
  } else {
    Serial.println("ERROR: serial value buffer not big enough");
    registerDataReadError();
    data_read_status = DATA_READ_IDLE;
  }
}

void SerialReaderLoggerComponent::setSerialValueBuffer(char* val) {
  resetSerialValueBuffer();
  strncpy(value_buffer, val, sizeof(value_buffer) - 1);
  value_buffer[sizeof(value_buffer)-1] = 0;
  value_charcounter = strlen(value_buffer);
}

void SerialReaderLoggerComponent::appendToSerialUnitsBuffer(byte b) {
  if (units_charcounter < sizeof(units_buffer) - 2) {
    units_buffer[units_charcounter] = (char) b;
    units_charcounter++;
  } else {
    Serial.println("ERROR: serial units buffer not big enough");
    registerDataReadError();
    data_read_status = DATA_READ_IDLE;
  }
}

void SerialReaderLoggerComponent::setSerialUnitsBuffer(char* u) {
  resetSerialUnitsBuffer();
  strncpy(units_buffer, u, sizeof(units_buffer) - 1);
  units_buffer[sizeof(units_buffer)-1] = 0;
  units_charcounter = strlen(units_buffer);
}
