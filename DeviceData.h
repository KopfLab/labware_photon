#pragma once
#include "device/DeviceMath.h"

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
  RunningStats value;
  RunningStats data_time;

  // clearing
  bool auto_clear;

  // output parameters
  int decimals; // what should the decimals be? (positive = decimals, negative = integers)
  char json[100]; // full data log text

  DeviceData() {
    variable[0] = 0;
    units[0] = 0;
    decimals = 0;
    auto_clear = true;
    clear(true);
  };

  DeviceData(char* var) : DeviceData() { setVariable(var); }
  DeviceData(int d) : DeviceData() { setDecimals(d); }
  DeviceData(char* var, int d) : DeviceData(var) { setDecimals(d); }

  // clearing
  void clear(bool all = false);
  void setAutoClear(bool clear);

  // data
  int getN();
  double getValue();
  double getStdDev();
  unsigned long getDataTime();
  void setVariable(char* var);
  void setNewestValue(double val);
  void setNewestValue(char* val, bool infer_decimals = false, int add_decimals = 1);
  void setNewestValueInvalid();
  void saveNewestValue(bool average); // set value based on current newest_value (calculate average if true)
  void setNewestDataTime(unsigned long dt);
  void setUnits(char* u);
  void setDecimals(int d);
  int getDecimals();

  // operations
  bool isVariableIdentical(char* comparison);
  bool isUnitsIdentical(char* comparison);

  // logging
  void assembleLog(bool include_time_offset); // assemble log (with our without time offset, in seconds)
  void assembleInfo(); // assemble data info
};

/** CLEARING **/

void DeviceData::clear(bool all) {
  if (auto_clear || all) {
    newest_value_valid = false;
    value.clear();
    data_time.clear();
  }
}

void DeviceData::setAutoClear(bool clear) {
  auto_clear = clear;
}

/** DATA **/

int DeviceData::getN() {
  return value.n;
}

double DeviceData::getValue() {
  return value.mean;
}

double DeviceData::getStdDev() {
  return value.getStdDev();
}

unsigned long DeviceData::getDataTime() {
  return (unsigned long) round(data_time.mean);
}

void DeviceData::setVariable(char* var) {
  strncpy(variable, var, sizeof(variable) - 1);
  variable[sizeof(variable)-1] = 0;
}

void DeviceData::setNewestValue(double val) {
  newest_value = val;
  newest_value_valid = true;
}

// @param add_decimals how many decimals to add tot he infered decimals (only matters if inferred)
void DeviceData::setNewestValue(char* val, bool infer_decimals, int add_decimals) {
  setNewestValue(atof(val));
  if (infer_decimals)
    decimals = find_number_of_decimals(val) + add_decimals;
}


void DeviceData::setNewestValueInvalid() {
  newest_value_valid = false;
}

void DeviceData::setNewestDataTime(unsigned long dt) {
  newest_data_time = dt;
}

void DeviceData::saveNewestValue(bool average) {
  if (newest_value_valid) {

    // clear/overwrite values if not averaging or data time has overflowed (for safety)
    if (!average || newest_data_time < getDataTime()) {
      if (newest_data_time < getDataTime())
        Serial.println("WARNING: data time has overflowed --> restarting value to avoid incorrect data");
      value.clear();
      data_time.clear();
    }

    // add new values
    value.add(newest_value);
    data_time.add(newest_data_time);

    // debug
    //Serial.printf("value add: %3.10f, datatime add: %lu\nvalue    : %3.10f, datatime    : %lu, stdev  : %.10f\n",
    //  newest_value, newest_data_time, getValue(), getDataTime(), getStdDev());

    #ifdef DATA_DEBUG_ON
      (average) ?
        Serial.print("INFO: new average value saved for ") :
        Serial.print("INFO: single value saved for ");
      (getN() > 1) ?
        getDataDoubleWithSigmaText(variable, getValue(), getStdDev(), units, getN(), -1, json, sizeof(json), PATTERN_KVSUN_SIMPLE, decimals) :
        getDataDoubleText(variable, getValue(), units, -1, json, sizeof(json), PATTERN_KVU_SIMPLE, decimals);
      Serial.printf("%s (data time = %Lu ms)\n", json, getDataTime());
    #endif
  } else {
    Serial.println("WARNING: newest value not valid and therefore not saved");
  }
}

void DeviceData::setUnits(char* u) {
  strncpy(units, u, sizeof(units) - 1);
  units[sizeof(units)-1] = 0;
}

void DeviceData::setDecimals(int d) {
  decimals = d;
}

int DeviceData::getDecimals() {
  return decimals;
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
  if (getN() > 1) {
    // have data
    (include_time_offset) ?
      getDataDoubleWithSigmaText(variable, getValue(), getStdDev(), units, getN(), millis() - getDataTime(), json, sizeof(json), PATTERN_KVSUNT_JSON, decimals) :
      getDataDoubleWithSigmaText(variable, getValue(), getStdDev(), units, getN(), json, sizeof(json), PATTERN_KVSUN_JSON, decimals);
  } else if (getN() == 1) {
    // have single data point (sigma is not meaningful)
    (include_time_offset) ?
      getDataDoubleText(variable, getValue(), units, getN(), millis() - getDataTime(), json, sizeof(json), PATTERN_KVUNT_JSON, decimals) :
      getDataDoubleText(variable, getValue(), units, getN(), json, sizeof(json), PATTERN_KVUN_JSON, decimals);
  } else {
    // no data
    getDataNullText(variable, json, sizeof(json), PATTERN_KV_JSON);
  }
}

void DeviceData::assembleInfo() {
  if (newest_value_valid) {
    // valid data
    getDataDoubleText(variable, newest_value, units, -1, json, sizeof(json), PATTERN_KVU_JSON, decimals);
  } else {
    // no valid data
    getDataNullText(variable, json, sizeof(json), PATTERN_KV_JSON);
  }
}
