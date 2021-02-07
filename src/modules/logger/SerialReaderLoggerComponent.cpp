#include "application.h"
#include "SerialReaderLoggerComponent.h"

void SerialReaderLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // initialize serial communication
    Serial.printlnf("INFO: initializing serial communication, baud rate '%ld'", serial_baud_rate);
    Serial1.begin(serial_baud_rate, serial_config);

    // empty serial read buffer
    while (Serial1.available()) Serial1.read();
}

void SerialReaderLoggerComponent::startDataRead() {
    DataReaderLoggerComponent::startDataRead();
}

void SerialReaderLoggerComponent::readData() {
    DataReaderLoggerComponent::readData();
}

void SerialReaderLoggerComponent::handleDataReadError() {
    DataReaderLoggerComponent::handleDataReadError();
}

void SerialReaderLoggerComponent::handleDataReadTimeout() {
    DataReaderLoggerComponent::handleDataReadTimeout();
}

void SerialReaderLoggerComponent::finishDataRead() {
    DataReaderLoggerComponent::finishDataRead();
}
