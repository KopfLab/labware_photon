#pragma once
#include "DeviceData.h"

struct DeviceDataSerial : public DeviceData  {

  // buffers
  char variable_buffer[25];
  int variable_charcounter;
  char value_buffer[25];
  int value_charcounter;
  char units_buffer[20];
  int units_charcounter;

  DeviceDataSerial() : DeviceData() {
    resetBuffers();
  };

  void resetBuffers(); // reset all buffers
  void appendToVariableBuffer (byte b);
  void setVariableBuffer (char* var);
  void appendToValueBuffer (byte b);
  void saveNewestValueBuffer(char* val);
  void appendToUnitsBuffer (byte b);
  void setUnitsBuffer(char* u);
  void storeVariable(); // store variable buffer in variable
  void storeReceivedDataTime(long dt); // store the received data time
  void storeValue(); // convert value buffer to double (need to call saveNewestValue to actually set or average it)
  void storeUnits(); // store untis buffer in units
  bool compareVariable(); // check if current variable buffer is identical to variable
  bool compareUnits(); // check if current units buffer is identical to units
};

void DeviceDataSerial::resetBuffers() {
  int i;
  for (i=0; i < sizeof(variable_buffer); i++) variable_buffer[i] = 0;
  for (i=0; i < sizeof(value_buffer); i++) value_buffer[i] = 0;
  for (i=0; i < sizeof(units_buffer); i++) units_buffer[i] = 0;
  variable_charcounter = 0;
  value_charcounter = 0;
  units_charcounter = 0;
}

void DeviceDataSerial::appendToVariableBuffer(byte b) {
  variable_buffer[variable_charcounter] = (char) b;
  variable_charcounter++;
}

void DeviceDataSerial::setVariableBuffer(char* var) {
  strncpy(variable_buffer, var, sizeof(variable_buffer) - 1);
  variable_buffer[sizeof(variable_buffer)-1] = 0;
}

void DeviceDataSerial::appendToValueBuffer(byte b) {
  value_buffer[value_charcounter] = (char) b;
  value_charcounter++;
}

void DeviceDataSerial::saveNewestValueBuffer(char* val) {
  strncpy(value_buffer, val, sizeof(value_buffer) - 1);
  value_buffer[sizeof(value_buffer)-1] = 0;
}

void DeviceDataSerial::appendToUnitsBuffer(byte b) {
  units_buffer[units_charcounter] = (char) b;
  units_charcounter++;
}

void DeviceDataSerial::setUnitsBuffer(char* u) {
  strncpy(units_buffer, u, sizeof(units_buffer) - 1);
  units_buffer[sizeof(units_buffer)-1] = 0;
}

void DeviceDataSerial::storeVariable() {
  setVariable(variable_buffer);
}

void DeviceDataSerial::storeReceivedDataTime(long dt) {
  newest_data_time = dt;
}

void DeviceDataSerial::storeValue() {
  newest_value = atof(value_buffer);
}

void DeviceDataSerial::storeUnits() {
  setUnits(units_buffer);
}

bool DeviceDataSerial::compareVariable() {
  if (strcmp(variable, variable_buffer) == 0) {
    return(true);
  } else {
    return(false);
  }
}

bool DeviceDataSerial::compareUnits() {
  if (strcmp(units, units_buffer) == 0) {
    return(true);
  } else {
    return(false);
  }
}
