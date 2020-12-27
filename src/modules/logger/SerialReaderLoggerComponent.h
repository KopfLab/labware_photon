#pragma once
#include "DataReaderLoggerComponent.h"

/* component */
class SerialReaderLoggerComponent : public DataReaderLoggerComponent
{

  protected:

  
  public:

    /*** constructors ***/
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : DataReaderLoggerComponent(id, ctrl, data_have_same_time_offset) {}

    /*** read data ***/
    virtual void startDataRead();
    virtual void readData();
    virtual void handleDataReadError();
    virtual void handleDataReadTimeout();
    virtual void finishDataRead();

};
