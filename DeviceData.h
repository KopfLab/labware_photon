#pragma once

// particle log webhook
#define DATA_LOG_WEBHOOK  "device_data_log"  // name of the webhoook name

// device data for spark cloud
#define DATA_LOG_MAX_CHAR 255  // spark.publish is limited to 255 chars of data
struct DeviceData {

  long data_time; // the time the data was recorded (in ms)
  long newest_data_time; // the last recorded datetime (in ms)
  char variable[25]; // the name of the data variable
  double value; // the recorded value
  double newest_value; // the last recorded value
  char units[20]; // the units the data is recorded in
  int n; // how many values / times have been averaged
  int digits; // how many digits?
  char data_log[STATE_LOG_MAX_CHAR]; // full data log text

  DeviceData() {
    variable[0] = 0;
    units[0] = 0;
    digits = 3;
    newValue();
  };

  // data
  void newValue();
  void setVariable(char* var);
  void setValue(bool average);
  void setValue(double val, bool average);
  void setValue(long dt, double val, bool average);
  void setUnits(char* u);

  // logging
  void assembleLog(bool include_time_offset); // assemble log (with our without time offset, in seconds)
  bool publishLog(); // send log to cloud
  virtual void finalize(bool publish); // finalize command (publish = whether to publish log)

};

void DeviceData::newValue() {
  value = 0;
  data_time = 0;
  n = 0;
}

void DeviceData::setVariable(char* var) {
  strncpy(variable, var, sizeof(variable) - 1);
  variable[sizeof(variable)-1] = 0;
}

void DeviceData::setValue(bool average) {
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
}

void DeviceData::setValue(double val, bool average) {
  newest_value = val;
  setValue(newest_data_time, val, average);
}

void DeviceData::setValue(long dt, double val, bool average) {
  newest_data_time = dt;
  setValue(val, average);
}

void DeviceData::setUnits(char* u) {
  strncpy(units, u, sizeof(units) - 1);
  units[sizeof(units)-1] = 0;
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
