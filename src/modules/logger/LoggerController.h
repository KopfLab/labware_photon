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
#include "LoggerInfo.h"
#include "LoggerCommand.h"
#include "LoggerData.h"
#include "LoggerDisplay.h"

// forward declaration for component
class LoggerComponent;

// controller class
class LoggerController {

  private:

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
    void (*data_callback)() = 0;

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

    // constructor
    LoggerController (const char *version, int reset_pin) : LoggerController(version, reset_pin, new LoggerDisplay()) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd) : LoggerController(version, reset_pin, lcd, new LoggerControllerState()) {}
    LoggerController (const char *version, int reset_pin, LoggerControllerState *state) : LoggerController(version, reset_pin, new LoggerDisplay(), state) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd, LoggerControllerState *state) : version(version), reset_pin(reset_pin), lcd(lcd), state(state) {
      eeprom_location = eeprom_start + sizeof(*state);
    }

    // add component NEW
    void addComponent(LoggerComponent* component);

    // init (setup)
    void init(); 
    virtual void initComponents();

    // update (loop)
    void update();

    // reset
    bool wasReset() { reset; }; // whether controller was started in reset mode

    // Logger name
    void captureName(const char *topic, const char *data);
    void setNameCallback(void (*cb)()); // assign a callback function

    // data information
    virtual bool isTimeForDataLogAndClear(); // whether it's time for a data reset and log (if logging is on) // FIXME
    virtual void clearData(bool all = false); // clear data fields // FIXME
    virtual void assembleDataInformation(); // FIXME
    void addToDataInformation(char* info);
    void setDataCallback(void (*cb)()); // assign a callback function

    // state management
    virtual size_t getStateSize() { return(sizeof(*state)); }
    virtual void loadState(bool reset);
    virtual void loadComponentsState(bool reset);
    virtual void saveState();
    virtual bool restoreState();

    // state change functions
    bool changeLocked(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);
    bool changeDataLoggingPeriod(int period, int type);
    bool changeDataReadingPeriod(int period);

    // particle command parsing functions
    void setCommandCallback(void (*cb)()); // assign a callback function
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand (); // parse a cloud command
    virtual void parseComponentsCommand(); // parse a cloud command in the components
    bool parseLocked();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseDataLoggingPeriod();
    bool parseDataReadingPeriod();
    bool parseReset();

    // command info to LCD display
    virtual void updateDisplayCommandInformation();
    virtual void assembleDisplayCommandInformation();
    virtual void showDisplayCommandInformation();

    // state info to LCD display
    virtual void updateDisplayStateInformation();
    virtual void updateDisplayComponentsStateInformation();
    virtual void assembleDisplayStateInformation();
    virtual void showDisplayStateInformation();

    // logger state variable
    virtual void updateLoggerStateVariable();
    virtual void assembleLoggerStateVariable();
    virtual void assembleLoggerComponentsStateVariable();
    void addToLoggerStateVariableBuffer(char* info);
    virtual void postLoggerStateVariable();

    // particle variables
    virtual void updateDataInformation(); // FIXME
    virtual void postDataInformation(); // FIXME

    // particle webhook data log
    virtual void logData(); 
    virtual void resetDataLog();
    virtual bool addToDataLogBuffer(char* info);
    virtual bool finalizeDataLog(bool use_common_time, unsigned long common_time = 0);
    virtual bool publishDataLog();

    // particle webhook state log
    virtual void assembleStartupLog(); 
    virtual void assembleStateLog(); 
    virtual bool publishStateLog(); 

};
