#pragma once
#include <vector>
#include "LoggerCommand.h"
#include "LoggerData.h"

// declare controller to include as member
class LoggerController;

// component class
class LoggerComponent
{

  protected:

    // controller
    LoggerController *ctrl;
 
    // lcd buffer
    char lcd_buffer[21];

    // state
    size_t eeprom_start;

    // time offset - whether all data have the same
    bool data_have_same_time_offset;

    // keep track of which data has been logged
    int first_data_log_index;
    int last_data_log_index;

  public:

    // component id
    const char *id;

    // data
    std::vector<LoggerData> data;

    // constructor
    LoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : id(id), ctrl(ctrl), data_have_same_time_offset(data_have_same_time_offset) {}

    /* data */
    // setup data vector - override in derived clases, has to return the new index
    virtual uint8_t setupDataVector(uint8_t start_idx);
    // clear collecetd data
    // @param all whether to clear all data or keep persistant data intact
    virtual void clearData(bool all);

    /* data logging */
    virtual void logData();
    virtual bool assembleDataLog();

    /* state management */
    void setEEPROMStart(size_t start);
    virtual size_t getStateSize();
    virtual void loadState(bool reset = false);
    virtual bool restoreState();
    virtual void saveState();

    // initialization
    virtual void init();

    // update
    virtual void update();

    // commands
    virtual bool parseCommand(LoggerCommand *command);

    // state display
    virtual void updateDisplayStateInformation();

    // state variable
    virtual void assembleLoggerStateVariable();

};
