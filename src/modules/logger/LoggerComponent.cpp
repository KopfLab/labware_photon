#include "application.h"
#include "LoggerComponent.h"
#include "LoggerController.h"

/*** setup ***/

// setup data vector - override in derived clases, has to return the new index
uint8_t LoggerComponent::setupDataVector(uint8_t start_idx) { 
    Serial.printf("INFO: constructing data vector for component '%s' starting at index %d\n", id, start_idx);
    return(start_idx); 
};

void LoggerComponent::init() {
    Serial.printf("INFO: initializing component '%s'...\n", id);
};

/*** loop ***/

void LoggerComponent::update() {
    // only run if this component and the whole controller are set up as data readers
    if (data_reader && ctrl->state->data_reader) {
        updateDataRead();
    }
}

/*** read data ***/

bool LoggerComponent::isManualDataReader() {
    return(ctrl->state->data_reading_period == READ_MANUAL);
}

void LoggerComponent::updateDataRead() {
    
    // start new data read?
    bool start_data_read = false;

    // process data read status
    if (data_read_status == DATA_READ_WAITING) {
        // read data
        readData();
    } else if (data_read_status == DATA_READ_COMPLETE) {
        // read is complete, finalize data
        finishDataRead();
    } else if (data_read_status == DATA_READ_ERROR) {
        // encountered an error
        handleDataReadError();
    } else if (data_read_status == DATA_READ_WAITING && (millis() - data_read_start) > ctrl->state->data_reading_period) {
        // encountered a timeout
        handleDataReadTimeout();
    } else if (data_read_status == DATA_READ_IDLE && (isManualDataReader() || (millis() - data_read_start) > ctrl->state->data_reading_period)) {
        // data reader is idle and either manual or it has been since the data read period
        data_read_status = DATA_READ_REQUEST;
    } else if (data_read_status == DATA_READ_REQUEST) {
        // new data read request
        startDataRead();
    }
}

void LoggerComponent::startDataRead() {
    #ifdef DATA_DEBUG_ON
        (isManualDataReader()) ?
            Serial.printf("INFO: starting data read for component '%s' (manual mode)", id) :
            Serial.printf("INFO: starting data read for component '%s' ", id);
        Serial.println(Time.format(Time.now(), "at %Y-%m-%d %H:%M:%S %Z\n"));
    #endif
    data_read_start = millis();
    data_read_status = DATA_READ_WAITING;
}

void LoggerComponent::readData() {
    data_read_status = DATA_READ_COMPLETE;
}

void LoggerComponent::handleDataReadError() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("ERROR: component '%s' encountered an error trying to read data at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    // go back to idle
    data_read_status = DATA_READ_IDLE;
}

void LoggerComponent::handleDataReadTimeout() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("INFO: data reading period exceeded for component '%s' at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    // go back to idle
    data_read_status = DATA_READ_IDLE;
}

void LoggerComponent::finishDataRead() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("INFO: finished data read for component '%s' at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    data_read_status = DATA_READ_IDLE;
}

/*** state management ***/

void LoggerComponent::setEEPROMStart(size_t start) { 
    eeprom_start = start; 
}

size_t LoggerComponent::getStateSize() { 
    return(0); 
}

void LoggerComponent::loadState(bool reset) {
    if (getStateSize() > 0) {
    if (!reset){
        Serial.printf("INFO: trying to restore state from memory for component '%s'\n", id);
        restoreState();
    } else {
        Serial.printf("INFO: resetting state for component '%s' back to default values\n", id);
        saveState();
    }
    }
};

bool LoggerComponent::restoreState(){ 
    return(false); 
};

void LoggerComponent::saveState(){ 

};

/*** command parsing ***/

bool LoggerComponent::parseCommand(LoggerCommand *command) {
    return(false);
};

/*** state changes ***/


/*** state info to LCD display ***/

void LoggerComponent::updateDisplayStateInformation() {
    
};

/*** logger state variable ***/

void LoggerComponent::assembleStateVariable() {

};

/*** logger data variable ***/

void LoggerComponent::assembleDataVariable() {
  for (int i=0; i<data.size(); i++) {
    data[i].assembleInfo();
    ctrl->addToDataVariableBuffer(data[i].json);
  }
}

/*** particle webhook data log ***/

void LoggerComponent::clearData(bool all) {
    #ifdef DATA_DEBUG_ON
    Serial.printf("INFO: clearing component '%s' data at ", id);
    Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    for (int i=0; i<data.size(); i++) data[i].clear(all);
};

void LoggerComponent::logData() {
    last_data_log_index = -1;
    // chunked data logging
    while (assembleDataLog()) {
        #ifdef CLOUD_DEBUG_ON
        Serial.printf("INFO: assembled data log for component '%s' from index %d to %d\n", id, first_data_log_index, last_data_log_index);
        #endif
        ctrl->publishDataLog();
    }
};

bool LoggerComponent::assembleDataLog() {

    // first reporting index
    first_data_log_index = last_data_log_index + 1;
    int i = first_data_log_index;

    // check if we're already done with all data
    if (i == data.size()) return(false);

    // check first next data (is there at least one with data?)
    bool something_to_report = false;
    for (; i < data.size(); i++) {
        if(data[i].getN() > 0) {
            // found data that has something to report
            something_to_report = true;
            break;
        }
    }

    // nothing to report
    if (!something_to_report) return(false);

    // reset the log & buffers
    ctrl->resetDataLog();

    // all data that fits
    for(i = i + 1; i < data.size(); i++) {
        if(data[i].assembleLog(!data_have_same_time_offset)) {
            if (ctrl->addToDataLogBuffer(data[i].json)) {
            // successfully added to buffer
            last_data_log_index = i;
            } else {
            // no more space - stop here for this log
            break;
            }
        }
    }

    // finalize data log
    if (data_have_same_time_offset) {
        unsigned long common_time = millis() - (unsigned long) data[first_data_log_index].getDataTime();
        return(ctrl->finalizeDataLog(true, common_time));
    } else {
        return(ctrl->finalizeDataLog(false));
    }
    return(true);
}
