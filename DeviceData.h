#pragma once

// particle log webhook
#define DATA_LOG_WEBHOOK  "device_data_log"  // name of the webhoook name

// device data for spark cloud
struct DeviceData {

  // data information
  char variable[25]; // the name of the data variable
  char units[20]; // the units the data is recorded in

  // newest data
  long newest_data_time; // the last recorded datetime (in ms)
  double newest_value; // the last recorded value
  bool newest_value_valid; // whether the newest value is valid

  // saved data
  long data_time; // the time the data was recorded (in ms)
  double value; // the recorded value
  int n; // how many values / times have been averaged

  // output parameters
  int digits; // how many digits?
  char data_log[100]; // full data log text

  DeviceData() {
    variable[0] = 0;
    units[0] = 0;
    digits = 3;
    resetValue();
  };

  // data
  void resetValue();
  void setVariable(char* var);
  void setNewestValue(double val);
  void setNewestValue(char* val);
  void setNewestValueInvalid();
  void saveNewestValue(bool average); // set value based on current newest_value (calculate average if true)
  void setNewestDataTime(long dt);
  void setUnits(char* u);

  // operations
  bool isVariableIdentical(char* comparison);
  bool isUnitsIdentical(char* comparison);

  // logging
  void assembleLog(bool include_time_offset); // assemble log (with our without time offset, in seconds)
  bool publishLog(); // send log to cloud
  virtual void finalize(bool publish); // finalize command (publish = whether to publish log)

};

/** DATA **/

void DeviceData::resetValue() {
  value = 0;
  newest_value = 0;
  newest_value_valid = false;
  data_time = 0;
  newest_data_time = 0;
  n = 0;
}

void DeviceData::setVariable(char* var) {
  strncpy(variable, var, sizeof(variable) - 1);
  variable[sizeof(variable)-1] = 0;
}

void DeviceData::setNewestValue(double val) {
  newest_value = val;
  newest_value_valid = true;
}

void DeviceData::setNewestValue(char* val) {
  setNewestValue(atof(val));
}


void DeviceData::setNewestValueInvalid() {
  newest_value_valid = false;
}

void DeviceData::setNewestDataTime(long dt) {
  newest_data_time = dt;
}

void DeviceData::saveNewestValue(bool average) {
  if (newest_value_valid) {
    if (average)  {
      // average value and data time
      data_time = (long) (n*data_time + newest_data_time)/(n + 1.0);
      value = (double) (n*value + newest_value)/(n+1.0);
      n++;
    } else {
      // single value
      data_time = newest_data_time;
      value = newest_value;
      n = 1;
    }
  } else {
    Serial.println("WARNING: newest value not valid and therefore not saved");
  }
}

void DeviceData::setUnits(char* u) {
  strncpy(units, u, sizeof(units) - 1);
  units[sizeof(units)-1] = 0;
}


/**** OPERATIONS ****/

bool DeviceData::isVariableIdentical(char* comparison) {
  if (strcmp(variable, comparison) == 0) {
    return(true);
  } else {
    return(false);
  }
}

bool DeviceData::isUnitsIdentical(char* comparison) {
  if (strcmp(units, comparison) == 0) {
    return(true);
  } else {
    return(false);
  }
}

/***** LOGGING *****/

void DeviceData::assembleLog(bool include_time_offset = true) {
  (include_time_offset) ?
    getDataDoubleTextWithTimeOffset(variable, value, units, n, data_time - millis(), data_log, sizeof(data_log), PATTERN_KVUNT_JSON, digits) :
    getDataDoubleText(variable, value, units, n, data_log, sizeof(data_log), PATTERN_KVUNT_JSON, digits);
  Serial.println("INFO: data log = " + String(data_log));
}

bool DeviceData::publishLog() {
  Serial.print("INFO: publishing data log to event '" + String(DATA_LOG_WEBHOOK) + "'... ");
  if(Particle.publish(DATA_LOG_WEBHOOK, data_log, PRIVATE, WITH_ACK)) {
    Serial.println("successful.");
    return(true);
  } else {
    Serial.println("failed!");
    return(false);
  }
}

void DeviceData::finalize(bool publish) {
  assembleLog();
  if (publish) publishLog();
}
