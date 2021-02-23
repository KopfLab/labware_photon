#pragma once
#include "MFCLoggerComponent.h"

/*** component ***/
class AlicatMFCLoggerComponent : public MFCLoggerComponent
{

  private:

    int gas_id = -1;
    char gas[10];

  public:

    /*** constructors ***/
    AlicatMFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state) : 
        MFCLoggerComponent(
            id, ctrl, state,
            /* baud rate */         19200,
            /* serial config */     SERIAL_8N1
        ) {}

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx);

    /*** manage data ***/
    virtual void processNewByte();
    virtual void finishData();

    /*** read data ***/
    virtual void sendSerialDataRequest();

};

