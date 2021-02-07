#pragma once
#include "DataReaderLoggerComponent.h"

/* component */
class SerialReaderLoggerComponent : public DataReaderLoggerComponent
{

  protected:

    // serial communication config
    const long serial_baud_rate;
    const long serial_config;
  
  public:

    /*** constructors ***/
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, const long baud_rate, const long serial_config) : 
      DataReaderLoggerComponent(id, ctrl, data_have_same_time_offset), serial_baud_rate(baud_rate), serial_config(serial_config) {}

    /*** setup ***/
    virtual void init();

    /*** read data ***/
    virtual void startDataRead();
    virtual void readData();
    virtual void handleDataReadError();
    virtual void handleDataReadTimeout();
    virtual void finishDataRead();

};
