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
#include "LoggerControllerState.h"
#include "LoggerConstants.h"
#include "LoggerCommand.h"
#include "LoggerData.h"
#include "LoggerDisplay.h"

// reset codes
#define RESET_UNDEF    1
#define RESET_RESTART  2
#define RESET_STATE    3

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
    std::vector<LoggerData> data;
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
