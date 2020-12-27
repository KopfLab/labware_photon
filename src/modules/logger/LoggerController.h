// debugging codes (define in main script to enable)
// - CLOUD_DEBUG_ON     // use to enable info messages about cloud variables
// - WEBHOOKS_DEBUG_ON // use to avoid cloud messages from getting sent
// - STATE_DEBUG_ON     // use to enable info messages about state changes
// - DATA_DEBUG_ON      // use to enable info messages about data changes
// - SERIAL_DEBUG_ON    // use to enable info messages about serial data
// - LCD_DEBUG_ON       // see LoggerDisplay.h
// - STATE_RESET        // use to force a state reset on startup

#pragma once
#include <vector>
#include "LoggerCommand.h"
#include "LoggerDisplay.h"

/*** spark cloud constants ***/
#define CMD_ROOT              "device" // command root (i.e. registered particle call function)
#define STATE_INFO_VARIABLE   "state" // name of the particle exposed state variable
#define STATE_INFO_MAX_CHAR   600 // how long is the state information maximally
#define STATE_LOG_WEBHOOK     "state_log"  // name of the webhook to Logger state log
#define STATE_LOG_MAX_CHAR    255  // spark.publish is limited to 255 chars of data
#define DATA_INFO_VARIABLE    "data" // name of the particle exposed data variable
#define DATA_INFO_MAX_CHAR    600 // how long is the data information maximally
#define DATA_LOG_WEBHOOK      "data_log"  // name of the webhook to Logger data log
#define DATA_LOG_MAX_CHAR     255  // spark.publish is limited to 255 chars of data

/*** commands ***/
// return codes:
//  -  0 : success without warning
//  - >0 : success with warnings
//  - <0 : failed with errors
#define CMD_RET_UNDEFINED                   -100 // undefined behavior
#define CMD_RET_SUCCESS                     0 // succes = 0
#define CMD_RET_ERR                         -1 // errors < 0
#define CMD_RET_ERR_TEXT                    "undefined error"
#define CMD_RET_ERR_LOCKED                  -2 // error locked
#define CMD_RET_ERR_LOCKED_TEXT             "locked"
#define CMD_RET_ERR_CMD                     -3 // invalid command
#define CMD_RET_ERR_CMD_TEXT                "invalid command"
#define CMD_RET_ERR_VAL                     -4 // invalid value
#define CMD_RET_ERR_VAL_TEXT                "invalid value"
#define CMD_RET_ERR_UNITS                   -5 // invalid units
#define CMD_RET_ERR_UNITS_TEXT              "invalid units"
#define CMD_RET_ERR_NOT_A_READER            -10 // not a data reader
#define CMD_RET_ERR_NOT_A_READER_TEXT       "logger is not a data reader"
#define CMD_RET_ERR_LOG_SMALLER_READ        -11 // log period cannot be smaller than read period!
#define CMD_RET_ERR_LOG_SMALLER_READ_TEXT   "log period must be larger than read period"
#define CMD_RET_ERR_READ_LARGER_MIN         -12 // read period cannot be smaller than its minimum
#define CMD_RET_ERR_READ_LARGER_MIN_TEXT    "read period must be larger than minimum"
#define CMD_RET_WARN_NO_CHANGE              1 // state unchaged because it was already the same
#define CMD_RET_WARN_NO_CHANGE_TEXT         "state already as requested"

// command log types
#define CMD_LOG_TYPE_UNDEFINED              "undefined"
#define CMD_LOG_TYPE_UNDEFINED_SHORT        "UDEF"
#define CMD_LOG_TYPE_ERROR                  "error"
#define CMD_LOG_TYPE_ERROR_SHORT            "ERR"
#define CMD_LOG_TYPE_STATE_CHANGED          "state changed"
#define CMD_LOG_TYPE_STATE_CHANGED_SHORT    "OK"
#define CMD_LOG_TYPE_STATE_UNCHANGED        "state unchanged"
#define CMD_LOG_TYPE_STATE_UNCHANGED_SHORT  "SAME"

// locking
#define CMD_LOCK            "lock" // device "lock on/off [notes]" : locks/unlocks the Logger
  #define CMD_LOCK_ON         "on"
  #define CMD_LOCK_OFF        "off"

// logging
#define CMD_STATE_LOG       "state-log" // device "state-log on/off [notes]" : turns state logging on/off
  #define CMD_STATE_LOG_ON     "on"
  #define CMD_STATE_LOG_OFF    "off"
#define CMD_DATA_LOG       "data-log" // device "data-log on/off [notes]" : turns data logging on/off
  #define CMD_DATA_LOG_ON     "on"
  #define CMD_DATA_LOG_OFF    "off"

// time units
#define CMD_TIME_MS     "ms" // milli seconds
#define CMD_TIME_SEC    "s"  // seconds
#define CMD_TIME_MIN    "m"  // minutes
#define CMD_TIME_HR     "h"  // hours
#define CMD_TIME_DAY    "d"  // days

// logging rate
#define CMD_DATA_LOG_PERIOD            "log-period" // device log-period number unit [notes] : timing between each data logging (if log is on)
  #define CMD_DATA_LOG_PERIOD_NUMBER   "x"          // device log-period 5x : every 5 data recordings
  #define CMD_DATA_LOG_PERIOD_SEC      CMD_TIME_SEC // device log-period 20s : every 20 seconds
  #define CMD_DATA_LOG_PERIOD_MIN      CMD_TIME_MIN // device log-period 3m : every 3 minutes
  #define CMD_DATA_LOG_PERIOD_HR       CMD_TIME_HR  // device log-period 1h : every hour

// reading rate
#define CMD_DATA_READ_PERIOD          "read-period" // device read-period number unit [notes] : timing between each data read, may not be smaller than device defined minimum and may not be smaller than log period (if a time)
  #define CMD_DATA_READ_PERIOD_MS     CMD_TIME_MS   // device read-period 200ms : read every 200 milli seconds
  #define CMD_DATA_READ_PERIOD_SEC    CMD_TIME_SEC  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MIN    CMD_TIME_MIN  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MANUAL "manual"      // read only upon manual trigger from the device (may not be available on all devices), typically most useful with 'log-period 1x'

// reset
#define CMD_RESET      "reset" 
  #define CMD_RESET_DATA  "data" // device "reset data" : reset the data stored in the device
  #define CMD_RESET_STATE "state" // device "reset state" : resets state (causes a device restart)

// restart
#define CMD_RESTART    "restart" // device "restart" : restarts the device

/*** reset codes ***/
#define RESET_UNDEF    1
#define RESET_RESTART  2
#define RESET_STATE    3

/*** state ***/

// log types
#define LOG_BY_TIME    0 // log period is a time in seconds
#define LOG_BY_EVENT   1 // log period is a number (x times)

// mnaual read constant
#define READ_MANUAL    0 // data reading is manual

struct LoggerControllerState {
  bool locked = false; // whether state is locked
  bool state_logging = false; // whether state is logged (whenever there is a change)
  bool data_logging = false; // whether data is logged
  uint data_logging_period ; // period between logs (in seconds!)
  uint8_t data_logging_type; // what the data logging period signifies
  bool data_reader = false; // whether this controller is a data reader
  uint data_reading_period_min; // minimum time between reads (in ms) [only relevant if it is a data_reader]
  uint data_reading_period; // period between reads (stored in ms!!!) [only relevant if it is a data_reader]
  uint8_t version = 3;

  LoggerControllerState() {};
  // without data reading settings
  LoggerControllerState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type) :
    locked(locked), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(false)  {}
  // with data reading settings
  LoggerControllerState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, uint data_reading_period_min, uint data_reading_period) :
    locked(locked), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(true), data_reading_period_min(data_reading_period_min), data_reading_period(data_reading_period)  {}

};

/*** class definition ***/

// forward declaration for component
class LoggerComponent;

// controller class
class LoggerController {

  private:

    // system reset & application watchdog
    const int reset_delay = 5000; // in ms - how long to delay the reset
    unsigned long reset_timer_start = 0; // start of the reset timer
    uint32_t trigger_reset = RESET_UNDEF; // what kind of reset to trigger
    uint32_t past_reset = RESET_UNDEF; // what kind of reset was triggered
    ApplicationWatchdog *wd;

    // reset PIN
    const int reset_pin;
    #ifdef STATE_RESET
      // force state reset
      bool reset = true;
    #else
      // no reset on startup
      bool reset = false;
    #endif

    // state log exceptions
    bool override_state_log = false;

    // logger info
    bool name_handler_registered = false;
    bool name_handler_succeeded = false;

    // cloud connection
    bool cloud_connection_started = false;
    bool cloud_connected = false;

    // mac address
    byte mac_address[6];

    // state info
    const size_t eeprom_start = 0;
    size_t eeprom_location = 0;

    // data indices
    uint8_t data_idx = 0;

  protected:

    // startup
    bool startup_logged = false;

    // lcd buffer
    char lcd_buffer[21];

    // call backs
    void (*name_callback)() = 0;
    void (*command_callback)() = 0;
    void (*state_update_callback)() = 0;
    void (*data_update_callback)() = 0;

    // buffer for date time
    char date_time_buffer[25];

    // buffer and information variables
    char state_variable[STATE_INFO_MAX_CHAR];
    char state_variable_buffer[STATE_INFO_MAX_CHAR-50];
    char data_information[DATA_INFO_MAX_CHAR];
    char data_information_buffer[DATA_INFO_MAX_CHAR-50];

    // buffers for log events
    char state_log[STATE_LOG_MAX_CHAR];
    char data_log[DATA_LOG_MAX_CHAR];
    char data_log_buffer[DATA_LOG_MAX_CHAR-10];

    // data logging tracker
    unsigned long last_data_log;

  public:

    // controller version
    const char *version;

    // public variables
    char name[20] = "";
    LoggerDisplay* lcd;
    LoggerControllerState* state;
    LoggerCommand* command = new LoggerCommand();
    std::vector<LoggerComponent*> components;
    std::vector<LoggerComponent*>::iterator components_iter;

    /*** constructors ***/
    LoggerController (const char *version, int reset_pin) : LoggerController(version, reset_pin, new LoggerDisplay()) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd) : LoggerController(version, reset_pin, lcd, new LoggerControllerState()) {}
    LoggerController (const char *version, int reset_pin, LoggerControllerState *state) : LoggerController(version, reset_pin, new LoggerDisplay(), state) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd, LoggerControllerState *state) : version(version), reset_pin(reset_pin), lcd(lcd), state(state) {
      eeprom_location = eeprom_start + sizeof(*state);
    }

    /*** callbacks ***/
    void setNameCallback(void (*cb)()); // callback executed after name retrieved from cloud
    void setCommandCallback(void (*cb)()); // callback executed after a command is received and processed
    void setStateUpdateCallback(void (*cb)()); // callback executed when state variable is updated
    void setDataUpdateCallback(void (*cb)()); // callback executed when data variable is updated

    /*** setup ***/
    void addComponent(LoggerComponent* component);
    void init(); 
    virtual void initComponents();

    /*** loop ***/
    void update();

    /*** logger name capture ***/
    void captureName(const char *topic, const char *data);

    /*** state management ***/
    virtual size_t getStateSize() { return(sizeof(*state)); }
    virtual void loadState(bool reset);
    virtual void loadComponentsState(bool reset);
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand (); // parse a cloud command
    virtual void parseComponentsCommand(); // parse a cloud command in the components
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseDataLoggingPeriod();
    bool parseDataReadingPeriod();
    bool parseReset();
    bool parseRestart();

    /*** state changes ***/
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);
    bool changeDataLoggingPeriod(int period, int type);
    bool changeDataReadingPeriod(int period);

    /*** command info to display ***/
    virtual void updateDisplayCommandInformation();
    virtual void assembleDisplayCommandInformation();
    virtual void showDisplayCommandInformation();

    /*** state info to LCD display ***/
    virtual void updateDisplayStateInformation();
    virtual void assembleDisplayStateInformation();
    virtual void showDisplayStateInformation();
    virtual void updateDisplayComponentsStateInformation();

    /*** logger state variable ***/
    virtual void updateStateVariable();
    virtual void assembleStateVariable();
    virtual void assembleComponentsStateVariable();
    void addToStateVariableBuffer(char* info);
    virtual void postStateVariable();

    /*** particle webhook state log ***/
    virtual void assembleStartupLog(); 
    virtual void assembleStateLog(); 
    virtual bool publishStateLog(); 

    /*** logger data variable ***/
    virtual void updateDataVariable();
    virtual void assembleComponentsDataVariable();
    void addToDataVariableBuffer(char* info);
    virtual void postDataVariable();

    /*** particle webhook data log ***/
    virtual bool isTimeForDataLogAndClear(); // whether it's time for data clear and log (if logging is on)
    virtual void clearData(bool all = false); // clear data fields
    virtual void logData(); 
    virtual void resetDataLog();
    virtual bool addToDataLogBuffer(char* info);
    virtual bool finalizeDataLog(bool use_common_time, unsigned long common_time = 0);
    virtual bool publishDataLog();

};
