#pragma once
#include "LoggerComponent.h"
#include "LoggerController.h"

// data read status codes
#define DATA_READ_IDLE      0 // data reader not engaged
#define DATA_READ_REQUEST   1 // make a read request
#define DATA_READ_WAITING   2 // waiting for (more) data
#define DATA_READ_COMPLETE  3 // all data received
#define DATA_READ_TIMEOUT   4 // read timed out

/* component */
class DataReaderLoggerComponent : public LoggerComponent
{

  protected:

    // data reader - whether this component is a data reader
    uint8_t data_read_status = DATA_READ_IDLE;
    unsigned long data_read_start = 0; // time the read started
    unsigned int error_counter = 0; // number of errors encountered during the read

  public:

    /*** constructors ***/
    // data clearing usually managed automatically -> set to true
    DataReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : LoggerComponent(id, ctrl, data_have_same_time_offset, true) {}

    /*** loop ***/
    virtual void update();

    /*** read data ***/
    virtual bool isManualDataReader();
    virtual void idleDataRead();
    virtual void initiateDataRead();
    virtual void readData();
    virtual void completeDataRead();
    virtual void registerDataReadError();
    virtual void handleDataReadTimeout();

    /*** manage data ***/
    virtual void startData();
    virtual void finishData();

};
