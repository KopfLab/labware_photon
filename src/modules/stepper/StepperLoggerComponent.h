#pragma once
#include "StepperConfig.h"
#include "ControllerLoggerComponent.h"
#include <AccelStepper.h>

/*** commands ***/

// status
#define CMD_START       "start" // device start [msg] : starts the stepper
#define CMD_STOP        "stop" // device stop [msg] : stops the stepper (stops power)
#define CMD_HOLD        "hold" // device hold [msg] : engage the stepper but not running (power on, no flow)
#define CMD_RUN         "run" // device run minutes [msg] : runs the stepper for x minutes
#define CMD_AUTO        "auto" // device auto [msg] : listens to external trigger on the trigger pin
#define CMD_ROTATE      "rotate" // device rotate number [msg] : run for x number of rotations

// direction
#define CMD_DIR         "direction" // device direction cw/cc/switch [msg] : set the direction
  #define CMD_DIR_CW      "cw"
  #define CMD_DIR_CC      "cc"
  #define CMD_DIR_SWITCH  "switch"

// speed
#define CMD_SPEED       "speed" // device speed number rpm/fpm [msg] : set the stepper speed
  #define SPEED_RPM       "rpm" // device speed number rpm [msg] : set speed in rotations per minute (requires step-angle)
#define CMD_RAMP        "ramp"  // device ramp number rpm/fpm minutes [msg] : ramp speed from current to [number] rpm/fpm over [minutes] of time

// microstepping
#define CMD_STEP        "ms" // device ms number/auto [msg] : set the microstepping
  #define CMD_STEP_AUTO   "auto" // signal to put microstepping into automatic mode (i.e. always pick the highest microstepping that the clockspeed supports)

// return codes
#define CMD_RET_WARN_MAX_RPM      101
#define CMD_RET_WARN_MAX_RPM_TEXT "exceeds max rpm"

/*** stepper component ***/
class StepperLoggerComponent : public ControllerLoggerComponent {

  private:

    // internal functions
    void construct();
    void updateStepper(bool init = false); // update stepper object
    float calculateSpeed(); // calculate speed based on settings
    int findMicrostepIndexForRpm(float rpm); // finds the correct ms index for the requested rpm (takes ms_auto into consideration)
    bool setSpeedWithSteppingLimit(float rpm); // sets state->speed and returns true if request set, false if had to set to limit

    // configuration
    StepperBoard* board;
    StepperDriver* driver;
    StepperMotor* motor;
    AccelStepper stepper;

    // state
    StepperState* state;

    // startup
    bool startup_rpm_logged = false;

    // debug
    bool debug_mode = false;

  public:

    /*** constructors ***/
    StepperLoggerComponent (const char *id, LoggerController *ctrl, StepperState* state, StepperBoard* board, StepperDriver* driver, StepperMotor* motor) : ControllerLoggerComponent(id, ctrl), state(state), board(board), driver(driver), motor(motor) {}

    /*** debug ***/
    void debug();

    /*** setup ***/
    uint8_t setupDataVector(uint8_t start_idx);
    void init();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();


    /*** command parsing ***/

    /*** state changes ***/

    /*

    void update(); // to be run during loop()

    float getMaxRpm(); // returns the maximum rpm for the pump

    bool changeDataLogging (bool on);
    bool changeStatus(int status);
    bool changeDirection(int direction);
    bool changeSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    bool changeToAutoMicrosteppingMode(); // set to automatic microstepping mode
    bool changeMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode

    bool start(); // start the pump
    bool stop(); // stop the pump
    bool hold(); // hold position
    long rotate(float number); // returns the number of steps the motor will take

    LoggerState* getDS() { return(ls); }; // return device state
    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

    void clearData(bool all) ;
    bool assembleDataLog();
    void logRpm();

    void assembleStateInformation();
    void updateStateInformation();

    void parseCommand();
    bool parseStatus();
    bool parseDirection();
    bool parseSpeed();
    bool parseMS();

    */

};
