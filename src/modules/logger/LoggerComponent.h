#pragma once
#include <vector>
#include "LoggerCommand.h"
#include "LoggerData.h"

// data read status codes
#define DATA_READ_IDLE      0 // data reader not engaged
#define DATA_READ_REQUEST   1 // make a read request
#define DATA_READ_WAITING   2 // waiting for (more) data
#define DATA_READ_COMPLETE  3 // all data received
#define DATA_READ_TIMEOUT   4 // read timed out
#define DATA_READ_ERROR     5 // encountered an error

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

    // data reader - whether this component is a data reader
    bool data_reader;
    uint8_t data_read_status = DATA_READ_IDLE;
    unsigned long data_read_start = 0;

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
    LoggerComponent (const char *id, LoggerController *ctrl, bool data_reader, bool data_have_same_time_offset) : id(id), ctrl(ctrl), data_reader(data_reader), data_have_same_time_offset(data_have_same_time_offset) {}

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx); // setup data vector - override in derived clases, has to return the new index
    virtual void init();

    /*** loop ***/
    void update();

    /*** read data ***/
    virtual bool isManualDataReader();
    virtual void updateDataRead();
    virtual void startDataRead();
    virtual void readData();
    virtual void handleDataReadError();
    virtual void handleDataReadTimeout();
    virtual void finishDataRead();

    /*** state management ***/
    void setEEPROMStart(size_t start);
    virtual size_t getStateSize();
    virtual void loadState(bool reset = false);
    virtual bool restoreState();
    virtual void saveState();

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
