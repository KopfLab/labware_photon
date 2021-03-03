#pragma once
#include "MFCLoggerComponent.h"

/*** serial mode ***/

#define MFC_SERIAL_MODE_IDLE         0
#define MFC_SERIAL_MODE_GAS          1
#define MFC_SERIAL_MODE_GAS_LIST     2
#define MFC_SERIAL_MODE_UNITS_START  3
#define MFC_SERIAL_MODE_UNITS        4
#define MFC_SERIAL_MODE_DATA         5

#define MFC_SWITCH_CHECK_TIMES       3 // how many times to check units are actually changing before making the change

/*** component ***/

class AlicatMFCLoggerComponent : public MFCLoggerComponent
{

  private:

    unsigned int serial_mode = MFC_SERIAL_MODE_IDLE ;
    bool switch_request = false; // request to switch units based on units data
    unsigned int units_switch_counter = MFC_SWITCH_CHECK_TIMES - 1; // get several confirmations about unit change, but first time switch right away
    unsigned int gas_switch_counter = 0; // get several confirmations about gas change
    unsigned int data_counter = 0; // keep track of data indices while processing the data

  public:

    int gas_id;
    char gas[10];

    /*** constructors ***/
    AlicatMFCLoggerComponent (const char *id, LoggerController *ctrl, MFCState* state) : 
        MFCLoggerComponent(
            id, ctrl, state,
            /* baud rate */         19200,
            /* serial config */     SERIAL_8N1
        ) {
          resetGas();
        }

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx);

    /*** state changes ***/
    virtual bool changeMFCID(char* mfc_id);

    /*** MFC functions ***/
    virtual void updateMFC(); // update the actual MFC when status or flow rate changes

    /*** loop ***/
    virtual void update();

    /*** manage serial data ***/
    virtual void sendSerialDataRequest();
    virtual void finishData();
    virtual void processNewByte();
    virtual void checkUnitID(char c);
    virtual void handleDataReadTimeout();

    /*** gas data ***/
    virtual void resetGas();
    virtual void processGas();
    virtual void processGasList();

    /*** units data ***/
    virtual void processUnitsStart();
    virtual void processUnits();
    virtual void checkUnit(unsigned int data_idx);

    /*** actual data **/
    virtual void processData();

    /*** logger data variable & particle webhook data log ***/
    virtual void addGasToUnits();
    virtual void removeGasFromUnits();
    virtual void assembleDataVariable();
    virtual void logData();

};

