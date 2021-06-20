#pragma once
#include "DataReaderLoggerComponent.h"
#include <OneWire.h>
#include <DS18.h>

/*** component ***/
class DS18B20TemperatureLoggerComponent : public DataReaderLoggerComponent
{

    // temp sensor
    const int one_wire_pin;
    OneWire* wire; // 1-wire 
    DS18* sensor; // temperature sensor
    bool sensor_found = false;
    byte sensor_addr[8];

  public:

    /*** constructors ***/
    // has global offset (true below), also defaults for DataReader: non-blocking reads, data clearing managed automatically
    DS18B20TemperatureLoggerComponent (const char *id, LoggerController *ctrl, int one_wire_pin) : 
        DataReaderLoggerComponent(id, ctrl, true), one_wire_pin(one_wire_pin) {}

    /*** setup ***/
    virtual void init();
    virtual uint8_t setupDataVector(uint8_t start_idx);

    /** loop **/
    virtual void update();

    /*** read data ***/
    virtual void readData();

    /*** sensor ***/
    bool foundSensor();

};

/*** setup ***/

void DS18B20TemperatureLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // initialize wire and sensor
    wire = new OneWire(one_wire_pin);  // 1-wire signal on pin D4
    sensor = new DS18(one_wire_pin);
}

uint8_t DS18B20TemperatureLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // add data: idx, key, units, digits
    data.push_back(LoggerData(start_idx + 1, "T", "DegC", 2));
    return(start_idx + data.size()); 
}

/*** loop ***/

void DS18B20TemperatureLoggerComponent::update() {
    if (!sensor_found) {
        if (wire->search(sensor_addr)) {
            // found something
            sensor_found = true;
            Serial.print("Info: found a");

            // the first ROM byte indicates which chip family
            switch (sensor_addr[0]) {
                case 0x10:
                    Serial.print(" DS1820/DS18S20 Temp sensor");
                    break;
                case 0x28:
                    Serial.print(" DS18B20 Temp sensor");
                    break;
                case 0x22:
                    Serial.print(" DS1822 Temp sensor");
                    break;
                case 0x26:
                    Serial.print(" DS2438 Smart Batt Monitor");
                    break;
                default:
                    Serial.println("n unknown Device type");
                    sensor_found = false;
            }

            // print out address
            Serial.print(" at address");
            for(int i = 0; i < 8; i++) {
                Serial.print(" 0x");
                Serial.print(sensor_addr[i],HEX);
            }
            Serial.println();
        } else {
            // nothing found -> reset to make sure to start over
            wire->reset_search();
        } 
        wire->reset(); // clear bus for next use
    }
    DataReaderLoggerComponent::update();
}

/*** read data ***/

void DS18B20TemperatureLoggerComponent::readData() {
    DataReaderLoggerComponent::readData();
    if (sensor_found) {
        if (sensor->read(sensor_addr)) {
            // retrieve sensor data
            data[0].setNewestValue(sensor->celsius());
        } else {
            // supposed to have a sensor but address didn't work
            ctrl->lcd->printLineTemp(1, "ERR: lost sensor");
            sensor_found = false;
        }
    } 
}

/*** sensor ***/
bool DS18B20TemperatureLoggerComponent::foundSensor() {
    return(sensor_found);
}