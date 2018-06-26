#pragma once

// device data for spark cloud
struct DeviceData {

  // data information
  char variable[25]; // the name of the data variable
  char units[20]; // the units the data is recorded in

  // newest data
  unsigned long newest_data_time; // the last recorded datetime (in ms)
  double newest_value; // the last recorded value
  bool newest_value_valid; // whether the newest value is valid

  // saved data
  unsigned long data_time; // the time the data was recorded (in ms)
  double value; // the recorded value
  int n; // how many values / times have been averaged

  // output parameters
  int digits; // how many digits?
  char json[100]; // full data log text

  DeviceData() {
    variable[0] = 0;
    units[0] = 0;
    digits = 0;
    resetValue();
  };

  DeviceData(char* var) : DeviceData() { setVariable(var); }
  DeviceData(int d) : DeviceData() { digits = d; }
  DeviceData(char* var, int d) : DeviceData(var) { digits = d; }

  // data
  void resetValue();
  void setVariable(char* var);
  void setNewestValue(double val);
  void setNewestValue(char* val);
  void setNewestValueInvalid();
  void saveNewestValue(bool average); // set value based on current newest_value (calculate average if true)
  void setNewestDataTime(unsigned long dt);
  void setUnits(char* u);

  // operations
  bool isVariableIdentical(char* comparison);
  bool isUnitsIdentical(char* comparison);

  // logging
  void assembleLog(bool include_time_offset); // assemble log (with our without time offset, in seconds)
  void assembleInfo(); // assemble data info
};

/** DATA **/

void DeviceData::resetValue() {
  value = 0;
  data_time = 0;
  n = 0;
  newest_value_valid = false;
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

void DeviceData::setNewestDataTime(unsigned long dt) {
  newest_data_time = dt;
}

void DeviceData::saveNewestValue(bool average) {
  if (newest_value_valid) {
    // average but only if data time has not overflowed! (safety check)
    if (average && newest_data_time >= data_time)  {
      // average value
      double weight1 = n/(n+1.0);
      double weight2 = 1.0/(n+1.0);
      value = weight1*value + weight2*newest_value;

      // average data time (data time averaging must be done with the type back and forth to avoid calculation errors)
      double dt1 = round(weight1 * (double) data_time);
      double dt2 = round(weight2 * (double) newest_data_time);
      data_time = (unsigned long) dt1 + (unsigned long) dt2;

      // increment n
      n++; // unlikely to overflow with regular reset

      #ifdef DATA_DEBUG_ON
        Serial.print("INFO: new average value saved for ");
      #endif
    } else {
      // safety check
      if (newest_data_time < data_time)
        Serial.println("WARNING: data time has overflowed --> restarting value to avoid incorrect data");

      // single value
      value = newest_value;
      data_time = newest_data_time;
      n = 1;
      #ifdef DATA_DEBUG_ON
        Serial.print("INFO: single value saved for ");
      #endif
    }
    #ifdef DATA_DEBUG_ON
      getDataDoubleText(variable, value, units, -1, json, sizeof(json), PATTERN_KVU_SIMPLE, digits);
      Serial.printf("%s (n=%d; data time = %Lu ms)\n", json, n, data_time);
    #endif
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
  if (n > 0) {
    // have data
    (include_time_offset) ?
      getDataDoubleText(variable, value, units, n, millis() - data_time, json, sizeof(json), PATTERN_KVUNT_JSON, digits) :
      getDataDoubleText(variable, value, units, n, json, sizeof(json), PATTERN_KVUN_JSON, digits);
  } else {
    // no data
    getDataNullText(variable, json, sizeof(json), PATTERN_KV_JSON);
  }
}

void DeviceData::assembleInfo() {
  if (newest_value_valid) {
    // valid data
    getDataDoubleText(variable, newest_value, units, -1, json, sizeof(json), PATTERN_KVU_JSON, digits);
  } else {
    // no valid data
    getDataNullText(variable, json, sizeof(json), PATTERN_KV_JSON);
  }
}
