#include "application.h"
#include "OpticalDensityLoggerComponent.h"

/*** setup ***/

void OpticalDensityLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // define the pins
    pinMode(led_pin, OUTPUT);
    pinMode(ref_pin, INPUT);
    pinMode(sig_pin, INPUT);
    updateBeam(state->beam);
}

uint8_t OpticalDensityLoggerComponent::setupDataVector(uint8_t start_idx) { 
    
    // add data: idx, key, units, digits
    data.push_back(LoggerData(start_idx + 1, "Absorbance", "OD630", 4)); // assumes the wavelenth is 630
    data.push_back(LoggerData(start_idx + 2, "Transmittance", "signal/zero", 4));
    data.push_back(LoggerData(start_idx + 3, "signal", "beam/ref", 4));
    data.push_back(LoggerData(start_idx + 4, "zero", "beam/ref", 4));
    data.push_back(LoggerData(start_idx + 5, "beam", "signal", 1));
    data.push_back(LoggerData(start_idx + 6, "beam", "bgrd", 1));
    data.push_back(LoggerData(start_idx + 7, "ref", "signal", 1));
    data.push_back(LoggerData(start_idx + 8, "ref", "bgrd", 1));
    return(start_idx + data.size()); 
}

/*** state management ***/

size_t OpticalDensityLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void OpticalDensityLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool OpticalDensityLoggerComponent::restoreState() {
    OpticalDensityState *saved_state = new OpticalDensityState();
    EEPROM.get(eeprom_start, *saved_state);
    bool recoverable = saved_state->version == state->version;
    if(recoverable) {
        EEPROM.get(eeprom_start, *state);
        Serial.printf("INFO: successfully restored component state from memory (state version %d)\n", state->version);
    } else {
        Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
        saveState();
    }
    return(recoverable);
}

void OpticalDensityLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}


/*** command parsing ***/

bool OpticalDensityLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseBeam(command)) {
    // beam state command parsed
  } else if (parseZero(command)) {
    // zero command parsed
  } else if (parseZeroNext(command)) {
    // zero next command parsed
  }
  return(command->isTypeDefined());
}

bool OpticalDensityLoggerComponent::parseBeam(LoggerCommand *command) {
  if (command->parseVariable(CMD_BEAM)) {
    // beam
    command->extractValue();
    if (command->parseValue(CMD_BEAM_ON)) {
      command->success(changeBeam(BEAM_ON));
    } else if (command->parseValue(CMD_BEAM_OFF)) {
      command->success(changeBeam(BEAM_OFF));
    } else if (command->parseValue(CMD_BEAM_AUTO)) {
      command->success(changeBeam(BEAM_AUTO));
    } else if (command->parseValue(CMD_BEAM_PAUSE)) {
      command->success(changeBeam(BEAM_PAUSE));
    }
  } 

  // set command data if type defined
  if (command->isTypeDefined()) {
    getOpticalDensityStateBeamInfo(state->beam, command->data, sizeof(command->data));
  }

  return(command->isTypeDefined());
}

bool OpticalDensityLoggerComponent::parseZero(LoggerCommand *command) {
  if (command->parseVariable(CMD_OD_ZERO)) {
    command->success(startZero());
  }
  return(command->isTypeDefined());
}

bool OpticalDensityLoggerComponent::parseZeroNext(LoggerCommand *command) {
  if (maxing && command->parseVariable(CMD_OD_ZERO_NEXT)) {
    command->success(continueZero());
  }
  return(command->isTypeDefined());
}

/*** state changes ***/

bool OpticalDensityLoggerComponent::changeBeam(int beam) {

  // only update if necessary
  bool changed = beam != state->beam;
  char info[20];
  getOpticalDensityStateBeamInfo(beam, info, sizeof(info), true);

  if (changed) {
    Serial.printlnf("INFO: %s beam updating to (%s)", id, info);  
    // always reset maxing
    maxing = false;
    // beam changes reset data logging
    ctrl->restartLastDataLog();
    clearData();
    if (beam == BEAM_OFF) {
      // switching beam off also resets zero
      state->is_zeroed = false;
    }
    state->beam = beam;
    saveState();
    updateBeam(state->beam);
    // make a read right away
    beam_read_status = BEAM_READ_IDLE;
    data_read_status = DATA_READ_REQUEST;
  } else {
     Serial.printlnf("INFO: %s beam unchanged (%s)", id, info);  
  }
  return(changed);

}

bool OpticalDensityLoggerComponent::startZero() {
  // clear all zeroing data and reset counter
  ref_zero_dark.clear();
  ref_zero.clear();
  sig_zero_dark.clear();
  sig_zero.clear();
  ratio_zero.clear();
  maxing = true;
  // make sure beam state is changed to auto when starting zero
  if (state->beam != BEAM_AUTO) changeBeam(BEAM_AUTO); 
  // but for maxing turn beam on (but not update state)
  updateBeam(BEAM_ON);
  // make a read right away
  data_read_status = DATA_READ_REQUEST;
  beam_read_status = BEAM_READ_IDLE;
  return(true);
}

bool OpticalDensityLoggerComponent::continueZero() {
  maxing = false;
  zeroing = true;
  zero_read_counter = 1;
  return(true);
}

void OpticalDensityLoggerComponent::changeZero() {
  // update state
  state->is_zeroed = true;
  state->ref_zero_dark.set(ref_zero_dark);
  state->ref_zero.set(ref_zero);
  state->sig_zero_dark.set(sig_zero_dark);
  state->sig_zero.set(sig_zero);
  state->ratio_zero.set(ratio_zero);
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(state->last_zero_datetime, sizeof(state->last_zero_datetime));
  saveState();
  ctrl->updateStateVariable();
  ctrl->restartLastDataLog();
  clearData();
}

bool OpticalDensityLoggerComponent::isMaxing() {
  return(maxing);
}

/*** beam management ***/

void OpticalDensityLoggerComponent::updateBeam(uint beam) {
    if (beam == BEAM_ON) {
        digitalWrite(led_pin, HIGH);
    } else if (beam == BEAM_OFF || beam == BEAM_PAUSE) {
        digitalWrite(led_pin, LOW);
    }
}

/*** read data ***/

void OpticalDensityLoggerComponent::returnToIdle() {
    DataReaderLoggerComponent::returnToIdle();
    beam_read_status = BEAM_READ_IDLE;
}


void OpticalDensityLoggerComponent::readData() {

    if (state->beam == BEAM_PAUSE) {
      // beam state PAUSED --> no reading, skip straight to complete
      Serial.printlnf("WARNING at %lu: beam is paused, not reading data, turn to AUTO to resume data reading", millis());
      data_read_status = DATA_READ_COMPLETE;
    } else if (beam_read_status == BEAM_READ_IDLE) {
      // idle -> start data
      startData();
      data_received_last = millis();
      // what is the state of the beam?
      if (maxing || state->beam == BEAM_ON) {
        // beam is ON (or maxxing during zero) --> read beam straight away
        if (debug_component) Serial.printlnf("DEBUG at %lu: beam is permanently ON - starting beam read", millis());
        beam_read_status = BEAM_READ_BEAM;
      } else if (state->beam == BEAM_OFF) {
        // beam is OFF --> read dark
        if (debug_component) Serial.printlnf("DEBUG at %lu: beam is permanently OFF - starting dark read", millis());
        beam_read_status = BEAM_READ_DARK;
      } else if (state->beam == BEAM_AUTO && (state->is_zeroed || zeroing)) {
        // beam is on AUTO --> start OD read sequence
        // stop stirrer if it's on
        if (stirrer && stirrer->state->status == STATUS_ON) {
          if (debug_component) Serial.printlnf("DEBUG at %lu: turning stirrer off", millis());
          stirrer->stepper.setSpeed(0); // stepper off
          stirrer->stepper.disableOutputs();
          stirrer_temp_off = true;
        }
        // start dark wait (cooldown)
        if (debug_component) Serial.printlnf("DEBUG at %lu: beam is on AUTO - starting dark wait (cooldown)", millis());
        updateBeam(BEAM_OFF);
        beam_read_status = BEAM_READ_WAIT_DARK;
      } else {
        // skip to finish read
        data_read_status = DATA_READ_COMPLETE;
      }
    } else if (beam_read_status == BEAM_READ_WAIT_DARK) {
      // wait for potential beam cooldown to finish
      if ((millis() - data_received_last) > state->warmup) {
        if (debug_component) Serial.printlnf("DEBUG at %lu: finished cooldown, starting dark read", millis());
        beam_read_status = BEAM_READ_DARK;
        data_received_last = millis();
      }
    } else if (beam_read_status == BEAM_READ_DARK) {
      // dark read --> read or if down move on
      if ((millis() - data_received_last) > state->read_length) {
        if (state->beam == BEAM_OFF) {
          // beam OFF --> wrap up the read
          if (debug_component) Serial.printlnf("DEBUG at %lu: finished dark read with beam permanently OFF", millis());
          data_read_status = DATA_READ_COMPLETE;
          beam_read_status = BEAM_READ_IDLE;
        } else if (state->beam == BEAM_AUTO) {
          // beam is on AUTO --> move on to warmp
          if (debug_component) Serial.printlnf("DEBUG at %lu: finished dark read , turning beam on for warmup", millis());
          updateBeam(BEAM_ON);
          beam_read_status = BEAM_READ_WAIT_BEAM;
          data_received_last = millis();
        }
      } else {
        collectDarkData();
      }
    } else if (beam_read_status == BEAM_READ_WAIT_BEAM) {
      // wait for beam warmup to finish
      if ((millis() - data_received_last) > state->warmup) {
        if (debug_component) Serial.printlnf("DEBUG at %lu: finished warmup, starting signal read", millis());
        beam_read_status = BEAM_READ_BEAM;
        data_received_last = millis();
      }
    } else if (beam_read_status == BEAM_READ_BEAM) {
      // read signal
      if ((millis() - data_received_last) > state->read_length) {
        if (debug_component) Serial.printlnf("DEBUG at %lu: finished beam read", millis());
        if (!maxing && state->beam == BEAM_AUTO) {
          // in AUTO mode -> update beam and stirrer
          updateBeam(BEAM_OFF); 
          if (stirrer && stirrer_temp_off && (!zeroing || zero_read_counter + 1 > zero_read_n)) {
            if (debug_component) Serial.printlnf("DEBUG at %lu: turning stirrer back on", millis());
            stirrer->stepper.enableOutputs();
            stirrer->stepper.setSpeed(stirrer->calculateSpeed()); // stepper back on
            stirrer_temp_off = false;
          }
        }
        data_read_status = DATA_READ_COMPLETE;
        beam_read_status = BEAM_READ_IDLE;
      } else {
        collectBeamData();
      }
    }
}

/*** manage data ***/

void OpticalDensityLoggerComponent::startData() {
  // reset temp data logs
  ref_dark.clear();
  ref_beam.clear();
  sig_dark.clear();
  sig_beam.clear();
  // process zeroing
  if (zeroing) {
    if (zero_read_counter == 1) Serial.printlnf("INFO: %lu - starting zeroing", millis());
    ctrl->lcd->resetBuffer();
    snprintf(ctrl->lcd->buffer, sizeof(ctrl->lcd->buffer), "zero read #%d", zero_read_counter);
    ctrl->lcd->printLineTempFromBuffer(2);
  } 
  // start data 
  DataReaderLoggerComponent::startData();
}

void OpticalDensityLoggerComponent::collectDarkData() {
  ref_dark.add(analogRead(ref_pin));
  sig_dark.add(analogRead(sig_pin));
}

void OpticalDensityLoggerComponent::collectBeamData() {
  ref_beam.add(analogRead(ref_pin));
  sig_beam.add(analogRead(sig_pin));
}

void OpticalDensityLoggerComponent::finishData() {

  // FIXME: deal with saturation of sensor during zero-ing!!
  if (state->beam == BEAM_OFF) {
    // beam is OFF --> not saving the data, keeping it just for LCD update
    if (debug_component) {
       Serial.printlnf(
          "REF DARK: %4.0f +/- %4.0f (%d)\nSIG DARK: %4.0f +/- %4.0f (%d)", 
          ref_dark.getMean(), ref_dark.getStdDev(), ref_dark.getN(),
          sig_dark.getMean(), sig_dark.getStdDev(), sig_dark.getN());
    }
    // make another read right away
    data_read_status = DATA_READ_REQUEST;
  } else if (maxing || state->beam == BEAM_ON) {
    // beam is ON --> not saving the data, keeping it just for LCD update
    if (debug_component) {
       Serial.printlnf(
          "REF BEAM: %4.0f +/- %4.0f (%d)\nSIG BEAM: %4.0f +/- %4.0f (%d)", 
          ref_beam.getMean(), ref_beam.getStdDev(), ref_beam.getN(),
          sig_beam.getMean(), sig_beam.getStdDev(), sig_beam.getN());
    }
    // make another read right away
    data_read_status = DATA_READ_REQUEST;
  } else if (state->beam == BEAM_AUTO) {
    // beam is AUTO
    if (zeroing) {
      ref_zero_dark.add(ref_dark.getMean()/ 4095.0 * 100.0);
      sig_zero_dark.add(sig_dark.getMean()/ 4095.0 * 100.0);
      ref_zero.add(ref_beam.getMean()/ 4095.0 * 100.0);
      sig_zero.add(sig_beam.getMean()/ 4095.0 * 100.0);
      ratio_zero.add((sig_beam.getMean() - sig_dark.getMean() - led_offset) / (ref_beam.getMean() - ref_dark.getMean() - led_offset));
      zero_read_counter++;
      if (zero_read_counter > zero_read_n) {
        zeroing = false;
        Serial.printlnf("INFO: %lu - finished zeroing", millis());
        if (debug_component) {
          Serial.printlnf(
            "REF BEAM: %4.0f +/- %4.0f (%d)\nREF DARK: %4.0f +/- %4.0f (%d)\nSIG BEAM: %4.0f +/- %4.0f (%d)\nSIG DARK: %4.0f +/- %4.0f (%d)\nRATIO: %4.4f +/- %4.4f (%d)", 
            ref_zero.getMean(), ref_zero.getStdDev(), ref_zero.getN(),
            ref_zero_dark.getMean(), ref_zero_dark.getStdDev(), ref_zero_dark.getN(),
            sig_zero.getMean(), sig_zero.getStdDev(), sig_zero.getN(),
            sig_zero_dark.getMean(), sig_zero_dark.getStdDev(), sig_zero_dark.getN(),
            ratio_zero.getMean(), ratio_zero.getStdDev(), ratio_zero.getN());
        }
        changeZero();
      } 
      // make another read right away
      data_read_status = DATA_READ_REQUEST;
    } else if (state->is_zeroed) {
      // storing actual data
      data[7].setNewestValue(ref_dark.getMean()/ 4095.0 * 100.0); data[7].saveNewestValue(true); // ref beam bgrd
      data[6].setNewestValue(ref_beam.getMean()/ 4095.0 * 100.0); data[6].saveNewestValue(true); // ref beam value
      data[5].setNewestValue(sig_dark.getMean()/ 4095.0 * 100.0); data[5].saveNewestValue(true); // sig beam bgrd
      data[4].setNewestValue(sig_beam.getMean()/ 4095.0 * 100.0); data[4].saveNewestValue(true); // sig beam value
      data[3].saveRunningStatsValue(state->ratio_zero); // sig/ref zero value - this is a good idea to include, otherwise measurements can't be reconstructed
      float ratio = (sig_beam.getMean() - sig_dark.getMean() - led_offset) / (ref_beam.getMean() - ref_dark.getMean() - led_offset);
      data[2].setNewestValue(ratio); data[2].saveNewestValue(true); // sig/ref beam value
      float transmittance = ratio / state->ratio_zero.getMean();
      data[1].setNewestValue(transmittance); data[1].saveNewestValue(true); // transmittance
      data[0].setNewestValue(-log10(transmittance)); data[0].saveNewestValue(true); // absorbance
    }
  }
}


/*** logger state variable ***/

void OpticalDensityLoggerComponent::assembleStateVariable() {
  char pair[80];
  getOpticalDensityStateBeamInfo(state->beam, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getOpticalDensityStateZeroedInfo(state->is_zeroed, state->last_zero_datetime, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  if (state->is_zeroed) {
    getStateDoubleText("zero-ratio", state->ratio_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 4, true); ctrl->addToStateVariableBuffer(pair);
    getStateDoubleText("zero-beam-signal", state->sig_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 2, true); ctrl->addToStateVariableBuffer(pair);
    getStateDoubleText("zero-beam-bgrd", state->sig_zero_dark.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 2, true); ctrl->addToStateVariableBuffer(pair);
    getStateDoubleText("zero-ref-signal", state->ref_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 2, true); ctrl->addToStateVariableBuffer(pair);
    getStateDoubleText("zero-ref-bgrd", state->ref_zero_dark.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 2, true); ctrl->addToStateVariableBuffer(pair);
  }
}
