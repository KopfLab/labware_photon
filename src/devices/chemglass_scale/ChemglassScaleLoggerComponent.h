#pragma once
#include "ScaleLoggerComponent.h"

/*** serial data parameters ***/

// pattern pieces
#define P_VAL       -1 // [ +-0-9]
#define P_UNIT      -2 // [GOC]
#define P_STABLE    -3 // [ S]
#define P_BYTE       0 // anything > 0 is specific byte

const int SCALE_DATA_PATTERN[] = {P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, P_VAL, ' ', ' ', P_UNIT, P_STABLE, SERIAL_B_CR, SERIAL_B_NL};

/*** component ***/
class ChemglassScaleLoggerComponent : public ScaleLoggerComponent
{

  public:

    /*** constructors ***/
    ChemglassScaleLoggerComponent (const char *id, LoggerController *ctrl, ScaleState* state) : 
        ScaleLoggerComponent(
            id, ctrl, state, 
            /* baud rate */             4800,
            /* serial config */         SERIAL_8N1,
            /* request command */       "#",
            /* data pattern size */     sizeof(SCALE_DATA_PATTERN) / sizeof(SCALE_DATA_PATTERN[0])
        ) {}

    /*** manage data ***/
    virtual void processNewByte();

};

/*** manage data ***/

void ChemglassScaleLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // pattern interpretation
    char c = (char) new_byte;
    if ( SCALE_DATA_PATTERN[data_pattern_pos] == P_VAL && ( (new_byte >= SERIAL_B_0 && new_byte <= SERIAL_B_9) || c == ' ' || c == '+' || c == '-' || c == '.') ) {
        if (c != ' ') appendToSerialValueBuffer(new_byte); // value (ignoring spaces)
    } else if (SCALE_DATA_PATTERN[data_pattern_pos] == P_UNIT && (c == 'G' || c == 'O' || c == 'C')) {
        // units
        if (c == 'G') setSerialUnitsBuffer("g"); // grams
        else if (c == 'O') setSerialUnitsBuffer("oz"); // ounces
        else if (c == 'C') setSerialUnitsBuffer("ct"); // what is ct??
        if (!data[0].isUnitsIdentical(units_buffer)) {
            // units are switching - clear all even persistent
            clearData(true);
            data[0].setUnits(units_buffer);
            //setRateUnits();//FIXME
        }
    } else if (SCALE_DATA_PATTERN[data_pattern_pos] == P_STABLE && (c == 'S' || c == ' ')) {
        // whether the reading is stable - note: not currently interpreted
    } else if (SCALE_DATA_PATTERN[data_pattern_pos] > P_BYTE && new_byte == SCALE_DATA_PATTERN[data_pattern_pos]) {
        // specific ascii characters
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
    }

    // next data pattern position
    data_pattern_pos++;
}

