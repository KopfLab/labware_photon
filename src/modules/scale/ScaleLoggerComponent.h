#pragma once
#include "SerialReaderLoggerComponent.h"

/* commands */
#define CMD_CALC_RATE          "calc-rate" // device calc-rate <unit> [notes] : whether to calculate rates and if so which time units they have
    #define CMD_CALC_RATE_OFF      "off"  // device calc-rate off : do not calculate rate
    #define CMD_CALC_RATE_SEC      CMD_TIME_SEC  // device calc-rate s : calculate the rate in units of [mass]/s
    #define CMD_CALC_RATE_MIN      CMD_TIME_MIN  // device calc-rate m : calculate the rate in units of [mass]/min
    #define CMD_CALC_RATE_HR       CMD_TIME_HR   // device calc-rate h : calculate the rate in units of [mass]/hr
    #define CMD_CALC_RATE_DAY      CMD_TIME_DAY  // device calc-rate d : calculate the rate in units of [mass]/day

// state values
#define CALC_RATE_OFF   0
#define CALC_RATE_SEC   1
#define CALC_RATE_MIN   60
#define CALC_RATE_HR    3600
#define CALC_RATE_DAY   86400

/* state */
struct ScaleState {

  uint calc_rate;
  uint8_t version = 1;

  ScaleState() {};

  ScaleState (uint calc_rate) : calc_rate(calc_rate) {};

};

/*** state variable formatting ***/

// logging_period (any pattern)
static void getStateCalcRateText(uint calc_rate, char* target, int size, char* pattern, bool include_key = true) {
  char units[10];
  if (calc_rate == CALC_RATE_OFF) strcpy(units, CMD_CALC_RATE_OFF);
  else if (calc_rate == CALC_RATE_SEC) strcpy(units, CMD_CALC_RATE_SEC);
  else if (calc_rate == CALC_RATE_MIN) strcpy(units, CMD_CALC_RATE_MIN);
  else if (calc_rate == CALC_RATE_HR) strcpy(units, CMD_CALC_RATE_HR);
  else if (calc_rate == CALC_RATE_DAY) strcpy(units, CMD_CALC_RATE_DAY);
  else {
    strcpy(units, "?");
    Serial.printf("ERROR: unknown calc rate setting %d\n", calc_rate);
  }
  getStateStringText(CMD_CALC_RATE, units, target, size, pattern, include_key);
}

// read period (standard patterns)
static void getStateCalcRateText(uint calc_rate, char* target, int size, bool value_only = false) {
  (value_only) ?
      getStateCalcRateText(calc_rate, target, size, PATTERN_V_SIMPLE, false) :
      getStateCalcRateText(calc_rate, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/* component */
class ScaleLoggerComponent : public SerialReaderLoggerComponent
{

  private:

    // debug
    bool debug_mode = false;

    // weight memory for rate calculation
    RunningStats prev_weight1;
    RunningStats prev_weight2;
    unsigned long prev_data_time1;
    unsigned long prev_data_time2;

  public:

    // state
    ScaleState* state;

    /*** constructors ***/
    // scale does not have global time offset since rate timestampe is beween two serial reads
    ScaleLoggerComponent (const char *id, LoggerController *ctrl, ScaleState* state, const long baud_rate, const long serial_config, const char *request_command, unsigned int data_pattern_size) : 
      SerialReaderLoggerComponent(id, ctrl, false, baud_rate, serial_config, request_command, data_pattern_size), state(state) {}
    ScaleLoggerComponent (const char *id, LoggerController *ctrl, ScaleState* state, const long baud_rate, const long serial_config, const char *request_command) : 
      ScaleLoggerComponent(id, ctrl, state, baud_rate, serial_config, request_command, 0) {}

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx); // setup data vector - override in derived clases, has to return the new index

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parseCalcRate(LoggerCommand *command);

    /*** state changes ***/
    bool changeCalcRate(uint rate);

    /*** rate calculations ***/
    void calculateRate();

    /*** manage data ***/
    virtual void finishData();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** particle webhook data log ***/
    virtual void clearData(bool clear_persistent = false);
    virtual void logData();

};