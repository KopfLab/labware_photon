#pragma once
#include "DataReaderLoggerComponent.h"

/* component */
class SerialReaderLoggerComponent : public DataReaderLoggerComponent
{

  protected:

    // serial communication config
    const long serial_baud_rate;
    const long serial_config;
  
    // serial data
    const char *request_command;
    unsigned int n_byte = 0;
    unsigned int data_pattern_pos = 0;
    unsigned int data_pattern_size = 0;
    byte new_byte;

    // buffers
    char data_buffer[500];
    int data_charcounter;
    char variable_buffer[25];
    int variable_charcounter;
    char value_buffer[25];
    int value_charcounter;
    char units_buffer[20];
    int units_charcounter;

  public:

    /*** constructors ***/
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, const long baud_rate, const long serial_config, const char *request_command, unsigned int data_pattern_size) : 
      DataReaderLoggerComponent(id, ctrl, data_have_same_time_offset), serial_baud_rate(baud_rate), serial_config(serial_config), request_command(request_command) {}
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, const long baud_rate, const long serial_config, const char *request_command) : 
      SerialReaderLoggerComponent(id, ctrl, data_have_same_time_offset, baud_rate, serial_config, request_command, 0) {}

    /*** setup ***/
    virtual void init();

    /*** read data ***/
    virtual void sendSerialDataRequest();
    virtual void idleDataRead();
    virtual void initiateDataRead();
    virtual void readData();
    virtual void completeDataRead();
    virtual void registerDataReadError();
    virtual void handleDataReadTimeout();

    /*** manage data ***/
    virtual void startData();
    virtual void processNewByte();
    virtual void finishData();

    /*** interact with serial data buffers ***/
    void resetSerialBuffers(); // reset all buffers
    void resetSerialDataBuffer();
    void resetSerialVariableBuffer();
    void resetSerialValueBuffer();
    void resetSerialUnitsBuffer();

    void appendToSerialDataBuffer (byte b); // append to total serial data buffer
    void appendToSerialVariableBuffer (byte b);
    void appendToSerialValueBuffer (byte b);
    void appendToSerialUnitsBuffer (byte b);

    void setSerialVariableBuffer (char* var);
    void setSerialValueBuffer(char* val);
    void setSerialUnitsBuffer(char* u);

};
