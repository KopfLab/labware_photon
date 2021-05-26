#pragma once
#include "LoggerMath.h"
#include "DataReaderLoggerComponent.h"
#include "StepperLoggerComponent.h"

/*** general commands ***/

#define CMD_OD_ZERO         "zero"      // device zero : zero the reader

#define CMD_BEAM            "beam"      // device beam [on/auto/off] : clarify the beam
  #define CMD_BEAM_ON         "on"      // beam permanently on (for troubleshooting)
  #define CMD_BEAM_OFF        "off"     // beam permanently off (no OD will be recorded, resets zero point)
  #define CMD_BEAM_AUTO       "auto"    // beam on/off as needed
  #define CMD_BEAM_PAUSE      "pause"   // beam paused (off) but will not reset the zero point

/*** general state ***/

#define BEAM_ON     1
#define BEAM_OFF    2
#define BEAM_AUTO   3
#define BEAM_PAUSE  4

struct OpticalDensityState {

    uint beam; //  on/off/auto
    uint read_length; // length of dark/beam reads (stored in ms)
    uint warmup; // length of warmup (stored in ms)
    bool is_zeroed;      // whether has been zerod
    char last_zero_datetime[30];  // last zero datetime
    RunningStats ref_zero_dark;
    RunningStats ref_zero;
    RunningStats sig_zero_dark;
    RunningStats sig_zero;
    RunningStats ratio_zero;
    
    uint8_t version = 1;

    OpticalDensityState() {};

    OpticalDensityState(uint beam, uint read_length, uint warmup) : 
        beam(beam), read_length(read_length), warmup(warmup), is_zeroed(false), 
        ref_zero_dark(RunningStats()), ref_zero(RunningStats()), sig_zero_dark(RunningStats()), sig_zero(RunningStats()), ratio_zero(RunningStats()) {};    

};

/*** state variable formatting ***/

// beam info
static void getOpticalDensityStateBeamInfo(unsigned int beam, char* target, int size, char* pattern, bool include_key = true)  {
  if (beam == BEAM_ON)
    getStateStringText("beam", "on", target, size, pattern, include_key);
  else if (beam == BEAM_OFF)
    getStateStringText("beam", "off", target, size, pattern, include_key);
  else if (beam == BEAM_AUTO)
    getStateStringText("beam", "auto", target, size, pattern, include_key);
  else if (beam == BEAM_PAUSE)
    getStateStringText("beam", "paused", target, size, pattern, include_key);
  else // should never happen
    getStateStringText("beam", "?", target, size, pattern, include_key);
}

static void getOpticalDensityStateBeamInfo(unsigned int beam, char* target, int size, bool value_only = false) {
  if (value_only) getOpticalDensityStateBeamInfo(beam, target, size, PATTERN_V_SIMPLE, false);
  else getOpticalDensityStateBeamInfo(beam, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// zero info
static void getOpticalDensityStateZeroedInfo(bool is_zeroed, char* zero_datetime, char* target, int size, char* pattern, bool include_key = true) {
  if (is_zeroed) {
    getStateStringText("zeroed", zero_datetime, target, size, pattern, include_key);
  } else {
    getDataNullText("zeroed", target, size, pattern);
  }
}

static void getOpticalDensityStateZeroedInfo(bool is_zeroed, char* zero_datetime, char* target, int size, bool value_only = false) {
  if (value_only) getOpticalDensityStateZeroedInfo(is_zeroed, zero_datetime, target, size, PATTERN_V_SIMPLE, false);
  else getOpticalDensityStateZeroedInfo(is_zeroed, zero_datetime, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/*** beam read status codes ***/

#define BEAM_READ_IDLE      0 // beam reader not engaged
#define BEAM_READ_WAIT_DARK 1 // waiting delay
#define BEAM_READ_DARK      2 // read dark signal
#define BEAM_READ_WAIT_BEAM 3 // waiting delay
#define BEAM_READ_BEAM      4 // read signal

/*** component ***/
class OpticalDensityLoggerComponent : public DataReaderLoggerComponent
{

  private:

    // config
    const int led_pin; // led PIN
    const int ref_pin; // reference PIN
    const int sig_pin; // signal PIN
    uint zero_read_n; // number of reads for zeroing

    // reading
    uint beam_read_status = BEAM_READ_IDLE;
    bool zeroing = false;
    uint zero_read_counter = 0;
    bool stirrer_temp_off = false;
    StepperLoggerComponent* stirrer = NULL;

    // zero
    RunningStats ref_zero_dark;
    RunningStats ref_zero;
    RunningStats sig_zero_dark;
    RunningStats sig_zero;
    RunningStats ratio_zero;

  public:

    // data
    RunningStats ref_dark;
    RunningStats ref_beam;
    RunningStats sig_dark;
    RunningStats sig_beam;

    // state
    OpticalDensityState* state;

    /*** constructors ***/
    // OpticalDensity has a global time offset
    OpticalDensityLoggerComponent (const char *id, LoggerController *ctrl, OpticalDensityState* state, int led_pin, int ref_pin, int sig_pin, uint zero_read_n, StepperLoggerComponent* stirrer) : 
        DataReaderLoggerComponent(id, ctrl, true), state(state), led_pin(led_pin), ref_pin(ref_pin), sig_pin(sig_pin), stirrer(stirrer), zero_read_n(zero_read_n) {}
    OpticalDensityLoggerComponent (const char *id, LoggerController *ctrl, OpticalDensityState* state, int led_pin, int ref_pin, int sig_pin, uint zero_read_n) : 
        OpticalDensityLoggerComponent(id, ctrl, state, led_pin, ref_pin, sig_pin, zero_read_n, NULL) {}

    /*** setup ***/
    virtual void init();
    virtual uint8_t setupDataVector(uint8_t start_idx);

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    virtual bool parseCommand(LoggerCommand *command);
    virtual bool parseBeam(LoggerCommand *command);
    virtual bool parseZero(LoggerCommand *command);

    /*** state changes ***/
    virtual bool changeBeam(int beam);
    virtual bool startZero();
    virtual void changeZero();

    /*** beam management ***/
    virtual void updateBeam(uint beam);

    /*** read data ***/
    virtual void readData();

    /*** manage data ***/
    virtual void startData();
    void collectDarkData();
    void collectBeamData();
    virtual void finishData();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

};