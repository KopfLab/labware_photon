#pragma once
#include "StirrerLoggerComponent.h"

/*** serial data parameters ***/
const int STIRRER_DATA_PATTERN[] = {'S', 'S', SERIAL_P_DIGIT, SERIAL_B_CR};

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
            /* data pattern size */     sizeof(STIRRER_DATA_PATTERN) / sizeof(STIRRER_DATA_PATTERN[0]),
            /* min RPM */               50,
            /* max RPM */               750,
            /* RPM change threshold */  1.0
        ) {}

    /*** manage data ***/
    virtual void processNewByte();
    
    /*** stirrer functions ***/
    virtual void updateStirrer(); 

};

/*** manage data ***/

void JKemStirrerLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // check whether to move on from stayed on pattern
    moveStayedOnPattern();

    // process current pattern
    int pattern = STIRRER_DATA_PATTERN[data_pattern_pos];
    if ( pattern == SERIAL_P_DIGIT && matchesPattern(new_byte, pattern)) {
        // number value (don't move pattern forward, could be multiple numbers)
        appendToSerialValueBuffer(new_byte);
        stayOnPattern(pattern);
    } else if (pattern > 0 && new_byte == pattern) {
        // specific ascii characters
        data_pattern_pos++;
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
        data_pattern_pos++;
    }
}

/*** stirrer functions ***/

void JKemStirrerLoggerComponent::updateStirrer() {
    StirrerLoggerComponent::updateStirrer();
    if (state->status == STIRRER_STATUS_ON) {
        Serial.printlnf("INFO: switching stirrer %s ON to %.0f rpm", id, state->rpm);
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "SS%.0f\r", state->rpm);
        Serial1.print(cmd); 
        // FIXME: should there be a check whether this actually worked on the next data read? in case we have exceeded the valid min or max?
    } else if (state->status == STIRRER_STATUS_OFF) {
        Serial.printlnf("INFO: switching stirrer OFF");
        Serial1.print("SS0\r"); 
    } else if (state->status == STIRRER_STATUS_MANUAL) {
        Serial.printlnf("INFO: switching stirrer to MANUAL mode");
        Serial1.print("RM\r"); 
    }
}