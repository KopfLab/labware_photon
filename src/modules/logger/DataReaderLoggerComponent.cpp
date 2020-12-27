#include "application.h"
#include "DataReaderLoggerComponent.h"

/*** loop ***/

void DataReaderLoggerComponent::update() {
    
    // only run if the whole controller is set up as data reader
    if (ctrl->state->data_reader) {
        
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
}

/*** read data ***/

bool DataReaderLoggerComponent::isManualDataReader() {
    return(ctrl->state->data_reading_period == READ_MANUAL);
}

void DataReaderLoggerComponent::startDataRead() {
    #ifdef DATA_DEBUG_ON
        (isManualDataReader()) ?
            Serial.printf("INFO: starting data read for component '%s' (manual mode)", id) :
            Serial.printf("INFO: starting data read for component '%s' ", id);
        Serial.println(Time.format(Time.now(), "at %Y-%m-%d %H:%M:%S %Z\n"));
    #endif
    data_read_start = millis();
    data_read_status = DATA_READ_WAITING;
}

void DataReaderLoggerComponent::readData() {
    data_read_status = DATA_READ_COMPLETE;
}

void DataReaderLoggerComponent::handleDataReadError() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("ERROR: component '%s' encountered an error trying to read data at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    // go back to idle
    data_read_status = DATA_READ_IDLE;
}

void DataReaderLoggerComponent::handleDataReadTimeout() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("INFO: data reading period exceeded for component '%s' at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    // go back to idle
    data_read_status = DATA_READ_IDLE;
}

void DataReaderLoggerComponent::finishDataRead() {
    #ifdef DATA_DEBUG_ON
        Serial.printf("INFO: finished data read for component '%s' at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    #endif
    data_read_status = DATA_READ_IDLE;
}
