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
        } else if (data_read_status == DATA_READ_WAITING && (millis() - data_read_start) > ctrl->state->data_reading_period) {
            // encountered a timeout
            handleDataReadTimeout();
        } else if (data_read_status == DATA_READ_WAITING) {
            // read data
            readData();
        } else if (data_read_status == DATA_READ_IDLE && (isManualDataReader() || (millis() - data_read_start) > ctrl->state->data_reading_period)) {
            // data reader is idle and either manual or it has been since the data read period
            data_read_status = DATA_READ_REQUEST;
        } else if (data_read_status == DATA_READ_IDLE) {
            // idle data read but not yet time for a new request
            idleDataRead();
        } else if (data_read_status == DATA_READ_REQUEST) {
            // new data read request
            initiateDataRead();
        }

    }
}

/*** read data ***/

bool DataReaderLoggerComponent::isManualDataReader() {
    return(ctrl->state->data_reading_period == READ_MANUAL);
}

void DataReaderLoggerComponent::idleDataRead() {
    // manage what happens during idle

}

void DataReaderLoggerComponent::initiateDataRead() {
    if (ctrl->debug_data) {
        (isManualDataReader()) ?
            Serial.printf("DEBUG: starting data read for component '%s' (manual mode)", id) :
            Serial.printf("DEBUG: starting data read for component '%s' ", id);
        Serial.println(Time.format(Time.now(), "at %Y-%m-%d %H:%M:%S %Z"));
    }
    data_read_start = millis();
    data_read_status = DATA_READ_WAITING;
    error_counter = 0;
}

void DataReaderLoggerComponent::readData() {
    startData();
    data_read_status = DATA_READ_COMPLETE;
}

void DataReaderLoggerComponent::completeDataRead() {
    if (ctrl->debug_data) {
        Serial.printf("DEBUG: finished data read with %d errors for component '%s' at ", error_counter, id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    }
    data_read_status = DATA_READ_IDLE;
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
    // go back to idle
    data_read_status = DATA_READ_IDLE;
}

/*** manage data ***/

void DataReaderLoggerComponent::startData() {
    unsigned long start_time = millis();
    for (int i=0; i < data.size(); i++) data[i].setNewestDataTime(start_time);
}

void DataReaderLoggerComponent::finishData() {
    // extend in derived classes, typically only save values if error_count == 0
}