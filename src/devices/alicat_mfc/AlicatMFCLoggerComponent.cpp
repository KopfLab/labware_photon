#include "application.h"
#include "AlicatMFCLoggerComponent.h"

/*** serial data parameters ***/

// requests
const char* GAS_REQUEST = "$$R46";
const char* GAS_LIST_REQUEST = "??G*";
const char* UNITS_REQUEST = "??D*";

// pattern pieces
#define MFC_P_UNIT      -1 // A, B, F, K
#define MFC_P_D         -2 // [0-9] --> 48 - 57
#define MFC_P_A         -3 // general ascii character --> 32 - 126
#define MFC_P_BYTE       0 // anything > 0 is specific byte

// gas command: 'A   046 = 264'
const int MFC_GAS_REGISTER_PATTERN[] = {MFC_P_UNIT, ' ', ' ', ' ', '0', '4', '6', ' ', '=', ' ', MFC_P_D, SERIAL_B_CR};

// gas list command: 'A G07       He'
const int MFC_GAS_LIST_PATTERN[] = {MFC_P_UNIT, ' ', 'G', MFC_P_D, ' ', MFC_P_A, SERIAL_B_CR};

// units command: 'A D00 ID_ NAME______________________ TYPE_______ WIDTH NOTES___________________'
const int MFC_UNITS_PATTERN_START[] = {
    MFC_P_UNIT,' ','D','0','0',' ','I','D','_',' ',
    'N','A','M','E','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',' ',
    'T','Y','P','E','_','_','_','_','_','_','_',' ',
    'W','I','D','T','H',' ',
    'N','O','T','E','S','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_', SERIAL_B_CR};

// units data: 'A D02 002 Abs Press                  s decimal     7/3 007 02 barA             '
const int MFC_UNITS_PATTERN[] = {MFC_P_UNIT, ' ', MFC_P_A, SERIAL_B_CR};

// actual data: 'A +00836 +023.77 +000.00 +000.00 +000.00 +000.00     N2'
const int MFC_DATA_PATTERN[] = {MFC_P_UNIT, ' ', MFC_P_A, SERIAL_B_CR};

/*** setup ***/

uint8_t AlicatMFCLoggerComponent::setupDataVector(uint8_t start_idx) { 
    
    // resize data vector
    data.resize(5);

    // add data: idx, key
    data[0] = LoggerData(1, "P");
    data[1] = LoggerData(2, "T");
    data[2] = LoggerData(5, "flow"); // volumetric
    data[3] = LoggerData(3, "flow"); // mass flow
    data[4] = LoggerData(4, "setpoint"); // what the mass flow is supposed to be

    return(start_idx + data.size()); 
}

/*** state changes ***/

bool AlicatMFCLoggerComponent::changeMFCID (char* mfc_id) {
    bool changed = MFCLoggerComponent::changeMFCID (mfc_id);
    if (changed) resetGas();
    return(changed);
}

/*** MFC functions ***/
void AlicatMFCLoggerComponent::updateMFC() {
    MFCLoggerComponent::updateMFC();
    if (state->status == MFC_STATUS_ON) {
        Serial.printlnf("INFO: switching MFC %s ON to %.3f %s", state->mfc_id, state->setpoint, state->units);
        char cmd[20];
        snprintf(cmd, sizeof(cmd), "%sS%.2f\r", state->mfc_id, state->setpoint);
        Serial1.print(cmd); 
        // FIXME: should there be a check whether this actually worked on the next data read? in case we have exceeded the allowed max
        // could use Register 24 - Set Point to get a sense for where on the scale we are (what the setting is), 64000 is full scale so from that should be able to calculate the max_setpoint
    } else {
        Serial.printlnf("INFO: switching MFC %s OFF", state->mfc_id);
        Serial1.print(state->mfc_id);
        Serial1.print("S0\r"); 
    }
}

/*** loop ***/

void AlicatMFCLoggerComponent::update() {
    // check for gas
    if (serial_mode == MFC_SERIAL_MODE_IDLE && !update_mfc) {

        if (gas_id < 0 || gas[0] == '?') {
            // start serial requests looking for gas id and gas name
            serial_mode = MFC_SERIAL_MODE_GAS;
        } else {
            // start serial requests looking for units and data
            serial_mode = MFC_SERIAL_MODE_UNITS_START;
        }
    } 

    // run general update
    MFCLoggerComponent::update();
}

/*** manage serial data ***/

void AlicatMFCLoggerComponent::sendSerialDataRequest() {
    // decide what request to send over serial
    if (serial_mode == MFC_SERIAL_MODE_GAS) {
        // read the register that holds the gas information
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending gas command '%s%s' over serial connection for component '%s'", state->mfc_id, GAS_REQUEST, id);
        }
        data_pattern_size = sizeof(MFC_GAS_REGISTER_PATTERN) / sizeof(MFC_GAS_REGISTER_PATTERN[0]);
        Serial1.print(state->mfc_id);
        Serial1.print(GAS_REQUEST); 
        Serial1.print("\r"); 
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST) {
        // read the gas list
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending gas list command '%s%s' to find gas for gas ID '%d' for component '%s'", state->mfc_id, GAS_LIST_REQUEST, gas_id, id);
        }
        data_pattern_size = sizeof(MFC_GAS_LIST_PATTERN) / sizeof(MFC_GAS_LIST_PATTERN[0]);
        Serial1.print(state->mfc_id);
        Serial1.print(GAS_LIST_REQUEST); 
        Serial1.print("\r");
    } else if (serial_mode == MFC_SERIAL_MODE_UNITS_START) {
         // read the units
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending units command '%s%s' for component '%s'", state->mfc_id, UNITS_REQUEST, id);
        }
        data_pattern_size = sizeof(MFC_UNITS_PATTERN_START) / sizeof(MFC_UNITS_PATTERN_START[0]);
        Serial1.print(state->mfc_id);
        Serial1.print(UNITS_REQUEST); 
        Serial1.print("\r");
    } else if (serial_mode == MFC_SERIAL_MODE_DATA) {
        // read the data
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: sending data command '%s' for component '%s'", state->mfc_id, id);
        }
        data_pattern_size = sizeof(MFC_DATA_PATTERN) / sizeof(MFC_DATA_PATTERN[0]);
        data_counter = 0;
        Serial1.print(state->mfc_id);
        Serial1.print("\r");
    } else {
        returnToIdle();
    }
}

void AlicatMFCLoggerComponent::finishData() {
    if (serial_mode == MFC_SERIAL_MODE_GAS && error_counter == 0) {
        // get gas_id from gas register
        gas_id = atoi(value_buffer) % 256;
        if (ctrl->debug_data) {
            Serial.printlnf("DEBUG: identified gas ID: '%d'", gas_id);
        }
        // jump straight to gas list request
        data_read_start = 0;
        serial_mode = MFC_SERIAL_MODE_GAS_LIST;
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST && error_counter == 0) {
        // get gas from gas list data
        snprintf (gas, sizeof(gas), "%s", value_buffer);
        Serial.printlnf("INFO: identified gas: '%s'", gas);
        // jump striaght to units request
        data_read_start = 0;
        serial_mode = MFC_SERIAL_MODE_UNITS_START;
    } else if (serial_mode == MFC_SERIAL_MODE_UNITS && error_counter == 0) {
        // jump straight to data request
        if (units_switch_counter > 0) {
            // units switch request made, repeat units check before completing switch
            serial_mode = MFC_SERIAL_MODE_UNITS_START;
        } else {
            // units are stable --> request data
            serial_mode = MFC_SERIAL_MODE_DATA;
        }
        data_read_start = 0;
    } else if (serial_mode == MFC_SERIAL_MODE_DATA && error_counter == 0 && !update_mfc) {
        // check if gas is correct
        if (strcmp(gas, value_buffer) == 0) {
            // update state with setpoint
            if (data[4].newest_value == 0 && state->status == MFC_STATUS_ON) {
                Serial.printlnf("INFO: MFC %s setpoint turned to 0, saving 'off' status.", state->mfc_id);
                state->status = MFC_STATUS_OFF;
                saveState();
                ctrl->updateStateVariable();
            } else if (data[4].newest_value > 0 && (data[4].newest_value != state->setpoint || state->status == MFC_STATUS_OFF)) {
                Serial.printlnf("INFO: MFC %s setpoint changed from %.3f to %.3f, saving setpoint and 'on' status.", state->mfc_id, state->setpoint, data[4].newest_value);
                state->setpoint = data[4].newest_value;
                state->status = MFC_STATUS_ON;
                saveState();
                ctrl->updateStateVariable();
            }
            if (strcmp(data[4].units, state->units) != 0) {
                Serial.printlnf("INFO: MFC %s setpoint units changed from %s to %s, saving new setpoint units.", state->mfc_id, state->units, data[4].units);
                snprintf (state->units, sizeof(state->units), "%s", data[4].units);
                saveState();
                ctrl->updateStateVariable();
            }
            // correct gas --> safe data
            for (int i=0; i < data.size(); i++) data[i].saveNewestValue(true); // average for all valid data
            gas_switch_counter = 0;
            serial_mode = MFC_SERIAL_MODE_IDLE;
        } else {
            // incorrect gs
            if (gas_switch_counter == MFC_SWITCH_CHECK_TIMES - 1) {
                // found incorrect gas several times --> reset
                Serial.printf("WARNING: not the correct gas, expected '%s', found '%s'\n", gas, value_buffer);
                snprintf(ctrl->lcd->buffer, sizeof(ctrl->lcd->buffer), "MFC gas: %s!=%s", value_buffer, gas);
                ctrl->lcd->printLineTempFromBuffer(1);
                clearData(true);
                resetGas();
                gas_switch_counter = 0;
                serial_mode = MFC_SERIAL_MODE_GAS; // go read gas
            } else {
                // keep track of how many times we had an error --> read again
                gas_switch_counter++;
                Serial.printlnf("INFO: gas switch requested, counter at %d", gas_switch_counter);
                serial_mode = MFC_SERIAL_MODE_DATA; // go read data again
            }
            data_read_start = 0;
        }
    } else {
        serial_mode = MFC_SERIAL_MODE_IDLE;
    }
}

void AlicatMFCLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // mode specific processing
    if (serial_mode == MFC_SERIAL_MODE_GAS) {
        processGas();
    } else if (serial_mode == MFC_SERIAL_MODE_GAS_LIST) {
        processGasList();
    } else if (serial_mode == MFC_SERIAL_MODE_UNITS_START) {
        processUnitsStart();
    } else if (serial_mode == MFC_SERIAL_MODE_UNITS) {
        processUnits();
    } else if (serial_mode == MFC_SERIAL_MODE_DATA) {
        processData();
    }

}

void AlicatMFCLoggerComponent::checkUnitID(char c) {
    // should be the unit
    if ( c != state->mfc_id[0]) {
        Serial.printf("WARNING: not the correct unit, expected '%s', found '%s'\n", state->mfc_id, c);
        registerDataReadError();
        ctrl->lcd->printLineTemp(1, "MFC: wrong ID");
        returnToIdle();
    }
}

void AlicatMFCLoggerComponent::handleDataReadTimeout() {
    MFCLoggerComponent::handleDataReadTimeout();
    serial_mode = MFC_SERIAL_MODE_IDLE;
}

/*** gas data ***/

void AlicatMFCLoggerComponent::resetGas() {
    gas_id = -1;
    gas[0] = '?';
    gas[1] = 0;
    // also reset read mode to not get stuck
    serial_mode = MFC_SERIAL_MODE_IDLE;
}

void AlicatMFCLoggerComponent::processGas() {
    // check if end of MFC_P_D pattern
    if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_D && data_pattern_pos + 1 < data_pattern_size && 
            MFC_GAS_REGISTER_PATTERN[data_pattern_pos + 1] > 0 && new_byte == MFC_GAS_REGISTER_PATTERN[data_pattern_pos + 1]) {
        data_pattern_pos++;
    }

    // pattern position
    if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_UNIT ) {
        checkUnitID((char) new_byte);
        data_pattern_pos++;
    } else if ( MFC_GAS_REGISTER_PATTERN[data_pattern_pos] == MFC_P_D && (new_byte >= SERIAL_B_0 && new_byte <= SERIAL_B_9)) {
        // number value (don't move pattern forward, could be multiple), exclude spaces
        appendToSerialValueBuffer(new_byte);
    } else if (new_byte == SERIAL_B_CR) {
        // end of line
        data_read_status = DATA_READ_COMPLETE;
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
     // check if end of MFC_P_D or MFC_P_A pattern
    if ( (MFC_GAS_LIST_PATTERN[data_pattern_pos] == MFC_P_D || MFC_GAS_LIST_PATTERN[data_pattern_pos] == MFC_P_A) && 
            data_pattern_pos + 1 < data_pattern_size && 
            MFC_GAS_LIST_PATTERN[data_pattern_pos + 1] > 0 && new_byte == MFC_GAS_LIST_PATTERN[data_pattern_pos + 1]) {
        data_pattern_pos++;
    }

    // pattern position
    if ( MFC_GAS_LIST_PATTERN[data_pattern_pos] == MFC_P_UNIT ) {
        checkUnitID((char) new_byte);
        data_pattern_pos++;
    } else if ( MFC_GAS_LIST_PATTERN[data_pattern_pos] == MFC_P_D && (new_byte >= SERIAL_B_0 && new_byte <= SERIAL_B_9)) {
        // number value (don't move pattern forward, could be multiple)
        appendToSerialVariableBuffer(new_byte);
    } else if ( MFC_GAS_LIST_PATTERN[data_pattern_pos] == MFC_P_A && (new_byte >= SERIAL_B_C_START && new_byte <= SERIAL_B_C_END)) {
        // ascii value (don't move pattern forward, could be multiple)
        if (new_byte != ' ') appendToSerialValueBuffer(new_byte);
    } else if (new_byte == SERIAL_B_CR) {
        // end of line
        if (atoi(variable_buffer) == gas_id) {
            // found the right gas
            data_read_status = DATA_READ_COMPLETE;
        } else {
            // keep looking
            resetSerialBuffers();
            data_pattern_pos = 0;
            n_byte = 0;
        }
    } else if (MFC_GAS_LIST_PATTERN[data_pattern_pos] > 0 && new_byte == MFC_GAS_LIST_PATTERN[data_pattern_pos]) {
        // specific ascii characters
        data_pattern_pos++;
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
        data_pattern_pos++;
    }
}

/*** units data ***/

void AlicatMFCLoggerComponent::processUnitsStart() {
    // pattern position
    if ( MFC_UNITS_PATTERN_START[data_pattern_pos] == MFC_P_UNIT ) {
        checkUnitID((char) new_byte);
        data_pattern_pos++;
    } else if (MFC_UNITS_PATTERN_START[data_pattern_pos] == SERIAL_B_CR && new_byte == SERIAL_B_CR) {
        // end of line
        resetSerialBuffers();
        data_pattern_size = sizeof(MFC_UNITS_PATTERN) / sizeof(MFC_UNITS_PATTERN[0]);
        data_pattern_pos = 0;
        n_byte = 0;
        serial_mode = MFC_SERIAL_MODE_UNITS;
    } else if (MFC_UNITS_PATTERN_START[data_pattern_pos] > 0 && new_byte == MFC_UNITS_PATTERN_START[data_pattern_pos]) {
        // specific ascii characters
        data_pattern_pos++;
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
        data_pattern_pos++;
    }
}

void AlicatMFCLoggerComponent::processUnits() {

    // data controllers (new lines)
    if (new_byte == SERIAL_B_CR && prev_byte == SERIAL_B_CR) {
        // double new line --> end of message
        if (switch_request && units_switch_counter == MFC_SWITCH_CHECK_TIMES - 1) {
            // unit switch happened -> reset data
            clearData(true);
            units_switch_counter = 0;
            Serial.println("INFO: units switch complete");
            ctrl->lcd->printLineTemp(1, "MFC: units changed");
        } else if (switch_request) {
            // switch request -> increase counter
            units_switch_counter++;
            Serial.printlnf("INFO: unit switch requested, counter at %d", units_switch_counter);
        } else {
            // no switch request -> reset counter
            units_switch_counter = 0;
        }
        switch_request = false;
        data_read_status = DATA_READ_COMPLETE;

    } else if (prev_byte == SERIAL_B_CR) {
        // start of new line --> process units for different data indices
        if (strcmp(variable_buffer, "D02") == 0 && strncmp(value_buffer, "Abs Press", 9) == 0) {
            // pressure
            checkUnit(0);
        } else if (strcmp(variable_buffer, "D03") == 0 && strncmp(value_buffer, "Flow Temp", 9) == 0) {
            // temperature
            checkUnit(1);
        } else if (strcmp(variable_buffer, "D04") == 0 && strncmp(value_buffer, "Volu Flow", 9) == 0) {
            // volumetric flow
            checkUnit(2);
        } else if (strcmp(variable_buffer, "D05") == 0 && strncmp(value_buffer, "Mass Flow", 9) == 0) {
            // mass flow
            checkUnit(3);            
        } else if (strcmp(variable_buffer, "D06") == 0 && strncmp(value_buffer, "Mass Flow Setpt", 15) == 0) {
            // mass flow setpoint
            checkUnit(4);
        } 
        
        // read next line
        resetSerialBuffers();
        data_pattern_pos = 0;
        n_byte = 1;
    } 
    
    // data checks & capture
    if ( MFC_UNITS_PATTERN[data_pattern_pos] == MFC_P_UNIT ) {
        // check unit ID
        checkUnitID((char) new_byte);
        data_pattern_pos++;
    } else if (MFC_DATA_PATTERN[data_pattern_pos] > 0 && new_byte == MFC_DATA_PATTERN[data_pattern_pos]) {
        // specific ascii characters
        data_pattern_pos++;
    } else  if ( MFC_UNITS_PATTERN[data_pattern_pos] == MFC_P_A && (new_byte >= SERIAL_B_C_START && new_byte <= SERIAL_B_C_END)) {
        // capture data
        if (n_byte >= 3 && n_byte < 6) {
            // entry ID, e.g. D03
            appendToSerialVariableBuffer(new_byte);
        } else if (n_byte >= 11 && n_byte < 38) {
            // entry name e.g. Mass Flow
            appendToSerialValueBuffer(new_byte);
        } else if (n_byte >= 63) {
            // entry units e.g. 'Sml/m'
            if (strcmp(variable_buffer, "D03") == 0 && new_byte == 96) {
                // for T units, replace ` with deg
                appendToSerialUnitsBuffer('d');
                appendToSerialUnitsBuffer('e');
                appendToSerialUnitsBuffer('g');
            } else {
                appendToSerialUnitsBuffer(new_byte);
            }
        }
    }
}

void AlicatMFCLoggerComponent::checkUnit(unsigned int data_idx) {
    // remove trailing white spaces from units
    int end = strlen(units_buffer); 
    while (end > 0 && units_buffer[end - 1] == ' ') --end;
    units_buffer[end] = 0;

    // check if we have a new unit
    if (strcmp(data[data_idx].units, units_buffer) != 0) {
        switch_request = true;
        if (units_switch_counter == MFC_SWITCH_CHECK_TIMES - 1) {
            // units switch request often enough to switch
            Serial.printlnf("INFO: switching %s units from '%s' to '%s'", data[data_idx].variable, data[data_idx].units, units_buffer);
            data[data_idx].setUnits(units_buffer);
        }
    }
}

/*** actual data **/

void AlicatMFCLoggerComponent::processData() {

    if ( MFC_DATA_PATTERN[data_pattern_pos] == MFC_P_UNIT ) {
        // check unit ID
        checkUnitID((char) new_byte);
        data_pattern_pos++;
    } else if (MFC_DATA_PATTERN[data_pattern_pos] > 0 && new_byte == MFC_DATA_PATTERN[data_pattern_pos]) {
        // specific ascii characters
        data_pattern_pos++;
    } else if (new_byte == SERIAL_B_CR) {
        // end of line
        data_read_status = DATA_READ_COMPLETE;
    } else  if ( MFC_DATA_PATTERN[data_pattern_pos] == MFC_P_A && (new_byte >= SERIAL_B_C_START && new_byte <= SERIAL_B_C_END)) {
        // capture data
        if (new_byte == ' ') {
            // value delimiter reached --> process
            if (data_counter < data.size()) {
                bool valid_value = data[data_counter].setNewestValue(value_buffer, true, true, 1L);
                if (!valid_value) {
                    Serial.printf("WARNING: problem %d with serial data for %s value: %s\n", error_counter, data[data_counter].variable, value_buffer);
                    snprintf(ctrl->lcd->buffer, sizeof(ctrl->lcd->buffer), "MFC: %d value error", error_counter);
                    ctrl->lcd->printLineTempFromBuffer(1);
                    error_counter++;
                }
            }
            data_counter++;
            resetSerialValueBuffer();
        } else {
            // add to buffer
            // note: since this also captures the gas name, spaces in gas name would be trouble!
            appendToSerialValueBuffer(new_byte);
        }
    } else {
        // unrecognized character
        registerDataReadError();
    }

}

/*** logger data variable & particle webhook data log ***/

void AlicatMFCLoggerComponent::addGasToUnits() {
    for (int i=0; i<data.size(); i++) { 
        strcpy(data[i].aux, data[i].units);        
        snprintf(data[i].units, sizeof(data[i].units), "%s %s", data[i].aux, gas);
    }
}

void AlicatMFCLoggerComponent::removeGasFromUnits() {
    for (int i=0; i<data.size(); i++) {
        strcpy(data[i].units, data[i].aux);
    }
}

void AlicatMFCLoggerComponent::assembleDataVariable() {
    addGasToUnits();
    MFCLoggerComponent::assembleDataVariable();
    removeGasFromUnits();
}

void AlicatMFCLoggerComponent::logData() {
    addGasToUnits();
    MFCLoggerComponent::logData();
    removeGasFromUnits();
}
