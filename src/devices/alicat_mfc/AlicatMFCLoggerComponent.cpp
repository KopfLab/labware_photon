#include "application.h"
#include "AlicatMFCLoggerComponent.h"

/*** serial data parameters ***/

// pattern pieces
#define MFC_P_UNIT      -1 // A, B, F, K
#define MFC_P_D         -2 // [0-9] --> 48 - 57
#define MFC_P_BYTE       0 // anything > 0 is specific byte

// e.g. 'A   046 = 264'
const int MFC_GAS_REGISTER_PATTERN[] = {MFC_P_UNIT, ' ', ' ', ' ', '0', '4', '6', ' ', '=', ' ', MFC_P_D, SERIAL_B_CR};

/*** serial mode ***/
#define MFC_SERIAL_MODE_GAS      1
#define MFC_SERIAL_MODE_GAS_LIST 2
#define MFC_SERIAL_MODE_UNITS    3
#define MFC_SERIAL_MODE_DATA     4

/*** setup ***/

uint8_t AlicatMFCLoggerComponent::setupDataVector(uint8_t start_idx) { 
    
    // resize data vector
    data.resize(6);

    // add data: idx, key
    data[0] = LoggerData(1, "P");
    data[1] = LoggerData(2, "T", "DegC"); // assume this is always in degC
    data[2] = LoggerData(5, "flow"); // volumetric
    data[3] = LoggerData(3, "flow"); // mass flow
    data[4] = LoggerData(4, "setpoint"); // what the flow is supposed to be

    return(start_idx + data.size()); 
}

/*** manage data ***/

void AlicatMFCLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // note: has new line characters as part of the message (13) between each data block
    // note: it seems that the whole message ends with 13 13 (two returns in a row)


    // check if end of data pattern
    if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_D && data_pattern_pos + 1 < data_pattern_size && 
            MFC_GAS_REGISTER_PATTERN[data_pattern_pos + 1] > 0 && new_byte == MFC_GAS_REGISTER_PATTERN[data_pattern_pos + 1]) {
        data_pattern_pos++;
    }

    // pattern position
    if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_UNIT ) {
        // should be the unit
        if ( (char) new_byte != state->mfc_id[0]) {
          Serial.printf("WARNING: not the correct unit, expected '%s', found '%s'\n", state->mfc_id, (char) new_byte);
          registerDataReadError();
          ctrl->lcd->printLineTemp(1, "MFC: wrong unit");
          data_read_status = DATA_READ_IDLE; // reset right away
        }
        data_pattern_pos++;
    } else if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_D && (new_byte >= SERIAL_B_0 && new_byte <= SERIAL_B_9)) {
        // number value (don't move pattern forward, could be multiple)
        appendToSerialValueBuffer(new_byte);
    } else if (MFC_GAS_REGISTER_PATTERN[data_pattern_pos] > 0 && new_byte == MFC_GAS_REGISTER_PATTERN[data_pattern_pos]) {
        // specific ascii characters
        data_pattern_pos++;
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
        data_pattern_pos++;
    }

}

/*** read data ***/

void AlicatMFCLoggerComponent::sendSerialDataRequest() {
    // TODO:
    // move most functionality into the AlicatMFCLoggerComponent as it's not clear yet what might be extractable into a more generic class
    // --> maybe just the command structure for "id ..." and "flow ?? ??" and everything else into the alicat?
    // 
    Serial.println("Asking for devince information command");
    //Serial1.print("A??M*"); // ask for manufacturer data
    //Serial1.print("A??D*"); // ask for device configuration including units
    //Serial1.print("A??G*"); // gas list
    data_pattern_size = sizeof(MFC_GAS_REGISTER_PATTERN) / sizeof(MFC_GAS_REGISTER_PATTERN[0]);
    Serial1.print("A$$R46"); // read the register that holds the gas information
    Serial1.print("\r");
}

void AlicatMFCLoggerComponent::finishData() {
    // weight
    if (error_counter == 0) {
        Serial.printlnf("new value: '%d'", atoi(value_buffer) % 256);
    }
}
