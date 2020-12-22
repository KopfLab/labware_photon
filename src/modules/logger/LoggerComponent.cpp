#include "application.h"
#include "LoggerComponent.h"
#include "LoggerController.h"

// setup data vector - override in derived clases, has to return the new index
uint8_t LoggerComponent::setupDataVector(uint8_t start_idx) { 
    Serial.printf("INFO: constructing data vector for component '%s' starting at index %d\n", id, start_idx);
    return(start_idx); 
};

// clear collecetd data
// @param all whether to clear all data or keep persistant data intact
void LoggerComponent::clearData(bool all) {
    #ifdef DATA_DEBUG_ON
    Serial.printf("INFO: clearing component '%s' data at ", id);
    Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    for (int i=0; i<data.size(); i++) data[i].clear(all);
};

// log data
void LoggerComponent::logData() {
    last_data_log_index = -1;
    // chunk-wise data logging
    while (assembleDataLog()) {
    #ifdef CLOUD_DEBUG_ON
        Serial.printf("INFO: assembled data log from index %d to %d\n", first_data_log_index, last_data_log_index);
    #endif
        ctrl->publishDataLog();
    }
};

/* data logging */
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

/* state management */
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

// initialization
void LoggerComponent::init() {
    Serial.printf("INFO: initializing component '%s'...\n", id);
};

// update
void LoggerComponent::update() {

};

// commands
bool LoggerComponent::parseCommand(LoggerCommand *command) {
    return(false);
};

// state display
void LoggerComponent::updateDisplayStateInformation() {
    
};

// state variable
void LoggerComponent::assembleLoggerStateVariable() {

};