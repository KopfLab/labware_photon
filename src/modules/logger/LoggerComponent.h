#pragma once
#include <vector>
#include "LoggerCommands.h"
#include "LoggerData.h"

// declare controller to include as member
class LoggerController;

// component class
class LoggerComponent
{

  protected:

    // controller
    LoggerController *ctrl;
 
    // lcd buffer
    char lcd_buffer[21];

    // state
    size_t eeprom_start;

    // time offset - whether all data have the same
    bool data_have_same_time_offset;

    // keep track of which data has been logged
    int first_data_log_index;
    int last_data_log_index;

  public:

    // component id
    const char *id;

    // data
    std::vector<LoggerData> data;

    // constructor
    LoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : id(id), ctrl(ctrl), data_have_same_time_offset(data_have_same_time_offset) {}

    /* data */

    // set data index

    // setup data vector - override in derived clases, has to return the new index
    virtual uint8_t setupDataVector(uint8_t start_idx) { 
      Serial.printf("INFO: constructing data vector for component '%s' starting at index %d\n", id, start_idx);
      return(start_idx); 
    };

    // clear collecetd data
    // @param all whether to clear all data or keep persistant data intact
    virtual void clearData(bool all) {
      #ifdef DATA_DEBUG_ON
        Serial.printf("INFO: clearing component '%s' data at ", id);
        Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
      #endif
      for (int i=0; i<data.size(); i++) data[i].clear(all);
    };

    // log data
    virtual void logData() {
      last_data_log_index = -1;
      // chunk-wise data logging
      while (assembleDataLog()) {
        #ifdef CLOUD_DEBUG_ON
          Serial.printf("INFO: assembled data log from index %d to %d\n", first_data_log_index, last_data_log_index);
        #endif
        //ctrl->publishDataLog();//FIXME
      }
    };

    /* data logging */
    virtual bool assembleDataLog() {

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
      //ctrl->resetDataLog();//FIXME

      // all data that fits
      /*for(i = i + 1; i < data.size(); i++) {
        if(data[i].assembleLog(!data_have_same_time_offset)) {
          if (ctrl->addToDataLogBuffer(data[i].json)) {//FIXME
            // successfully added to buffer
            last_data_log_index = i;
          } else {
            // no more space - stop here for this log
            break;
          }
        }
      }*/

      // finalize data log
      /*if (data_have_same_time_offset) {
        unsigned long common_time = millis() - (unsigned long) data[first_data_log_index].getDataTime();
        return(ctrl->finalizeDataLog(true, common_time));//FIXME
      } else {
        return(ctrl->finalizeDataLog(false));
      }*/
      return(true);
    };

    /* state management */
    void setEEPROMStart(size_t start) { eeprom_start = start; };
    virtual size_t getStateSize() { return(0); }
    virtual void loadState(bool reset) {
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
    virtual bool restoreState(){ return(false); };
    virtual void saveState(){ };

    // initialization
    virtual void init() {
       Serial.printf("INFO: initializing component '%s'...\n", id);
    };

    // update
    virtual void update() {

    };

    // commands
    virtual bool parseCommand(LoggerCommand *command) {
      return(false);
    };

    // state display
    virtual void updateDisplayStateInformation() {
      
    };

    // state variable
    virtual void assembleLoggerStateVariable() {
    };

};
