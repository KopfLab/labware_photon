#pragma once
#include "StirrerLoggerComponent.h"

/*** serial data parameters ***/

// pattern pieces
#define P_NUMBER       -1 // [0-9]

const int STIRRER_DATA_PATTERN[] = {'S', 'S', P_NUMBER, SERIAL_B_CR};

/*** component ***/
class JKemStirrerLoggerComponent : public StirrerLoggerComponent
{

  public:

    /*** constructors ***/
    JKemStirrerLoggerComponent (const char *id, LoggerController *ctrl, StirrerState* state) : 
        StirrerLoggerComponent(
            id, ctrl, state, 
            /* baud rate */             9600,
            /* serial config */         SERIAL_8N1,
            /* request command */       "SS\r",
            /* data pattern size */     sizeof(STIRRER_DATA_PATTERN) / sizeof(STIRRER_DATA_PATTERN[0])
        ) {}

    /*** manage data ***/
    virtual void processNewByte();
    
};

/*** manage data ***/

void JKemStirrerLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // next data pattern position
    data_pattern_pos++;
}

