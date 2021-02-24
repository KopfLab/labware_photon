#pragma once
#include "MFCLoggerComponent.h"

/*** serial mode ***/

#define MFC_SERIAL_MODE_IDLE     0
#define MFC_SERIAL_MODE_GAS      1
#define MFC_SERIAL_MODE_GAS_LIST 2
#define MFC_SERIAL_MODE_UNITS    3
#define MFC_SERIAL_MODE_DATA     4

/*** component ***/

class AlicatMFCLoggerComponent : public MFCLoggerComponent
{

  private:

    unsigned int serial_mode = MFC_SERIAL_MODE_IDLE;
    int gas_id = -1;
    char gas[10];

  public:

    /*** constructors ***/
    AlicatMFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state) : 
        MFCLoggerComponent(
            id, ctrl, state,
            /* baud rate */         19200,
            /* serial config */     SERIAL_8N1
        ) {
          gas[0] = 0;
        }

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx);

    /*** loop ***/
    virtual void update();

    /*** manage data ***/
    virtual void processNewByte();
    virtual void processGas();
    virtual void processGasList();
    virtual void finishData();

    /*** read data ***/
    virtual void sendSerialDataRequest();

};

