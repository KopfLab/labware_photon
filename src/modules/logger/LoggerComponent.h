#pragma once
#include <vector>
#include "LoggerCommand.h"
#include "LoggerData.h"

// forward declaration for controller
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

    /*** constructors ***/
    LoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : id(id), ctrl(ctrl), data_have_same_time_offset(data_have_same_time_offset) {}

    /*** setup ***/
    uint8_t setupDataVector(uint8_t start_idx); // setup data vector - override in derived clases, has to return the new index
    void init();

    /*** loop ***/
    void update();

    /*** state management ***/
    void setEEPROMStart(size_t start);
    size_t getStateSize();
    virtual void loadState(bool reset = false);
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    virtual bool parseCommand(LoggerCommand *command);

    /*** state changes ***/
    // implement in derived classes

    /*** state info to LCD display ***/
    virtual void updateDisplayStateInformation();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** logger data variable ***/
    virtual void assembleDataVariable();

    /*** particle webhook data log ***/
    virtual void clearData(bool all); // clear collected data // @param "all" whether to clear all data or keep persistant data intact
    virtual void logData();
    virtual bool assembleDataLog();

};
