#pragma once
#include "LoggerComponentDataReader.h"

/* component */
class LoggerComponentSerialReader : public LoggerComponentDataReader
{

  protected:

  
  public:

    /*** constructors ***/
    LoggerComponentSerialReader (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : LoggerComponentDataReader(id, ctrl, data_have_same_time_offset) {}

    /*** read data ***/
    virtual void startDataRead();
    virtual void readData();
    virtual void handleDataReadError();
    virtual void handleDataReadTimeout();
    virtual void finishDataRead();

};
