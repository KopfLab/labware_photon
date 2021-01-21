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

void LoggerComponent::completeStartup() {
    Serial.printf("INFO: completing startup for component '%s'...\n", id);
}

/*** loop ***/

void LoggerComponent::update() {
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

void LoggerComponent::saveState(){ 

};

bool LoggerComponent::restoreState(){ 
    return(false); 
};

void LoggerComponent::resetState() {

};

/*** command parsing ***/

bool LoggerComponent::parseCommand(LoggerCommand *command) {
    return(false);
};

/*** state changes ***/

void LoggerComponent::activateDataLogging() {
    
};

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

void LoggerComponent::clearData(bool clear_persistent) {
    if (auto_clear_data) {
        if (ctrl->debug_data) {
            (clear_persistent) ?
                Serial.printf("DEBUG: clearing all component '%s' data at ", id):
                Serial.printf("DEBUG: clearing only non-persistant component '%s' data at ", id);
            Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
        }
        for (int i=0; i<data.size(); i++) data[i].clear(clear_persistent);
    }
};

void LoggerComponent::logData() {
    last_data_log_index = -1;
    // chunked data logging
    while (assembleDataLog()) {
        if (ctrl->debug_cloud) {
            Serial.printf("DEBUG: assembled data log for component '%s' from index %d to %d\n", id, first_data_log_index, last_data_log_index);
        }
        ctrl->queueDataLog();
    }
};

bool LoggerComponent::assembleDataLog() {

    // first reporting index
    first_data_log_index = last_data_log_index + 1;
    
    // check if we're already done with all data
    if (first_data_log_index >= data.size()) return(false);

    // check first next data (is there at least one with data?)
    bool something_to_report = false;
    int i = first_data_log_index;
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
    i = first_data_log_index;
    for(; i < data.size(); i++) {
        if(data[i].assembleLog(!data_have_same_time_offset)) {
            if (!ctrl->addToDataLogBuffer(data[i].json)) {
                // no more space - stop here for this log
                break;
            }
        }
        last_data_log_index = i;
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
