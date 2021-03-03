#include "application.h"
#include "DataReaderLoggerComponent.h"

/*** loop ***/

void DataReaderLoggerComponent::update() {
    
    // only run if the whole controller is set up as data reader
    if (ctrl->state->data_reader) {
        
        // process data read status
        if (data_read_status == DATA_READ_COMPLETE) {
            // read is complete, finalize data
            completeDataRead();
        } else if (data_read_status == DATA_READ_WAITING && isTimedOut()) {
            // encountered a timeout
            handleDataReadTimeout();
        } else if (data_read_status == DATA_READ_WAITING) {
            // read data
            readData();
        } else if (data_read_status == DATA_READ_IDLE && (!sequential || !ctrl->sequential_data_read_in_progress) && isTimeForRequest()) {
            // it's time for data read request
            data_read_status = DATA_READ_REQUEST;
        } else if (data_read_status == DATA_READ_IDLE) {
            // idle data read but not yet time for a new request
            idleDataRead();
            // keep track of idle time for sequential readers
            if (sequential && ctrl->sequential_data_idle_start == 0) ctrl->sequential_data_idle_start = millis(); 
        } else if (data_read_status == DATA_READ_REQUEST && (!sequential || !ctrl->sequential_data_read_in_progress)) {
            // new data read request
            initiateDataRead();
        }

    }
}

/*** read data ***/

bool DataReaderLoggerComponent::isManualDataReader() {
    // check if dat reading period is manual
    return(ctrl->state->data_reading_period == READ_MANUAL);
}

bool DataReaderLoggerComponent::isTimeForRequest() {
    // reader is not sequential or controller is idle overall and this data reader is either manual or it has been enough time since the data read period
    return((!sequential || ctrl->sequential_data_idle_start > 0) && (isManualDataReader() || (millis() - data_read_start) > ctrl->state->data_reading_period));
}

bool DataReaderLoggerComponent::isTimedOut() {
    // whether the reader is timed out - by default if it's been longer than data_reading_period
    return((millis() - data_read_start) > ctrl->state->data_reading_period);
}

void DataReaderLoggerComponent::returnToIdle() {
    // return to idle and update sequential data read in progress
    data_read_status = DATA_READ_IDLE;
    if (sequential) ctrl->sequential_data_read_in_progress = false;
}

void DataReaderLoggerComponent::idleDataRead() {
    // manage what happens during idle
}

void DataReaderLoggerComponent::initiateDataRead() {
    if (ctrl->debug_data) {
        if (sequential) {
            (isManualDataReader()) ?
                Serial.printf("DEBUG: starting data read for sequential component '%s' (manual mode) ", id) :
                Serial.printf("DEBUG: starting data read for sequential component '%s' ", id);
        } else {
            (isManualDataReader()) ?
                Serial.printf("DEBUG: starting data read for parallel component '%s' (manual mode) ", id) :
                Serial.printf("DEBUG: starting data read for parallel component '%s' ", id);
        }
        Serial.printf("at %ld / ", millis());
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    }
    // keep track of sequential readers' activity
    if (sequential) {
        ctrl->sequential_data_read_in_progress = true;
        ctrl->sequential_data_idle_start = 0;
    }
    data_read_start = millis();
    data_received_last = millis();
    data_read_status = DATA_READ_WAITING;
    error_counter = 0;
}

void DataReaderLoggerComponent::readData() {
    startData();
    data_received_last = millis();
    data_read_status = DATA_READ_COMPLETE;
}

void DataReaderLoggerComponent::completeDataRead() {
    if (ctrl->debug_data) {
        Serial.printf("DEBUG: finished data read with %d errors for component '%s' at ", error_counter, id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    }
    returnToIdle();
    finishData();
    ctrl->updateDataVariable();
}

void DataReaderLoggerComponent::registerDataReadError() {
    error_counter++;
    Serial.printf("ERROR: component '%s' encountered an error (#%d) trying to read data at ", id, error_counter);
    Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    ctrl->lcd->printLineTemp(1, "ERR: data read error");
}

void DataReaderLoggerComponent::handleDataReadTimeout() {
    Serial.printf("WARNING: data reading period exceeded with %d errors for component '%s' at ", error_counter, id);
    Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    ctrl->lcd->printLineTemp(1, "ERR: timeout read");
    returnToIdle();
}

/*** manage data ***/

void DataReaderLoggerComponent::startData() {
    unsigned long start_time = millis();
    for (int i=0; i < data.size(); i++) data[i].setNewestDataTime(start_time);
}

void DataReaderLoggerComponent::finishData() {
    // extend in derived classes, typically only save values if error_count == 0
}