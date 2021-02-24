#include "application.h"
#include "AlicatMFCLoggerComponent.h"

/*** serial data parameters ***/

// requests
const char* GAS_REQUEST = "A$$R46";
const char* GAS_LIST_REQUEST = "A??G*";

// pattern pieces
#define MFC_P_UNIT      -1 // A, B, F, K
#define MFC_P_D         -2 // [0-9] --> 48 - 57
#define MFC_P_BYTE       0 // anything > 0 is specific byte

// gas command: 'A   046 = 264'
const int MFC_GAS_REGISTER_PATTERN[] = {MFC_P_UNIT, ' ', ' ', ' ', '0', '4', '6', ' ', '=', ' ', MFC_P_D, SERIAL_B_CR};

// gas list command: 'A G07       He'
const int MFC_GAS_LIST_PATTERN[] = {MFC_P_UNIT, ' ', 'G', MFC_P_D, SERIAL_B_CR}; // FIXME: continue here

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

/*** loop ***/

void AlicatMFCLoggerComponent::update() {
    // check for gas
    if (serial_mode == MFC_SERIAL_MODE_IDLE && gas_id < 0) {
        serial_mode = MFC_SERIAL_MODE_GAS;
    } else if (serial_mode == MFC_SERIAL_MODE_IDLE && strlen(gas) == 0) {
        serial_mode = MFC_SERIAL_MODE_GAS_LIST;
    }

    // run general update
    MFCLoggerComponent::update();
}

/*** manage data ***/

void AlicatMFCLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // note: has new line characters as part of the message (13) between each data block
    // note: it seems that the whole message ends with 13 13 (two returns in a row)

    // process gas
    if (serial_mode == MFC_SERIAL_MODE_GAS) {
        processGas();
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST) {
        processGasList();
    }

}

void AlicatMFCLoggerComponent::processGas() {
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

void AlicatMFCLoggerComponent::processGasList() {
}

/*** read data ***/

void AlicatMFCLoggerComponent::sendSerialDataRequest() {
    // TODO:
    // move most functionality into the AlicatMFCLoggerComponent as it's not clear yet what might be extractable into a more generic class
    // --> maybe just the command structure for "id ..." and "flow ?? ??" and everything else into the alicat?
    // 
    
    //Serial1.print("A??M*"); // ask for manufacturer data
    //Serial1.print("A??D*"); // ask for device configuration including units
    //Serial1.print("A??G*"); // gas list

    if (serial_mode == MFC_SERIAL_MODE_GAS) {
        // read the register that holds the gas information
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending gas command '%s' over serial connection for component '%s'", GAS_REQUEST, id);
        }
        data_pattern_size = sizeof(MFC_GAS_REGISTER_PATTERN) / sizeof(MFC_GAS_REGISTER_PATTERN[0]);
        Serial1.print(GAS_REQUEST); 
        Serial1.print("\r"); 
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST) {
        // read the gas list
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending gas list command '%s' to find gas for gas ID '%d' for component '%s'", GAS_LIST_REQUEST, gas_id, id);
        }
        //data_pattern_size = sizeof(MFC_GAS_REGISTER_PATTERN) / sizeof(MFC_GAS_REGISTER_PATTERN[0]);
        Serial1.print(GAS_LIST_REQUEST); 
        Serial1.print("\r");
    } else {
        data_read_status = DATA_READ_IDLE;
    }
}

void AlicatMFCLoggerComponent::finishData() {
    if (serial_mode == MFC_SERIAL_MODE_GAS && error_counter == 0) {
        // get gas_id from gas register
        gas_id = atoi(value_buffer) % 256;
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: identified gas ID: '%d'", gas_id);
        }
        // skip time delay and jump straight to gas list request
        data_read_status = DATA_READ_REQUEST;
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST) {
        // get gas from gas list data

    }
    serial_mode = MFC_SERIAL_MODE_IDLE;
}
