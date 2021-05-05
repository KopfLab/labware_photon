#include "application.h"
#include "OpticalDensityLoggerComponent.h"

/*** setup ***/

void OpticalDensityLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // define the pins
    pinMode(led_pin, OUTPUT);
    pinMode(ref_pin, INPUT);
    pinMode(sig_pin, INPUT);
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
    // parse zero command parsed
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

/*** state changes ***/

bool OpticalDensityLoggerComponent::changeBeam(int beam) {

  // only update if necessary
  bool changed = beam != state->beam;
  char info[20];
  getOpticalDensityStateBeamInfo(beam, info, sizeof(info), true);

  if (changed) {
    Serial.printlnf("INFO: %s beam updating to (%s)", id, info);  
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
  } else {
     Serial.printlnf("INFO: %s beam unchanged (%s)", id, info);  
  }
  return(changed);

}

bool OpticalDensityLoggerComponent::startZero() {
  ref_zero_dark.clear();
  ref_zero.clear();
  sig_zero_dark.clear();
  sig_zero.clear();
  ratio_zero.clear();
  zeroing = true;
  zero_read_counter = 1;
  data_read_status = DATA_READ_REQUEST;
  beam_read_status = BEAM_READ_IDLE;
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

/*** beam management ***/

void OpticalDensityLoggerComponent::updateBeam(uint beam) {
    if (beam == BEAM_ON) {
        digitalWrite(led_pin, HIGH);
    } else if (beam == BEAM_OFF || beam == BEAM_PAUSE) {
        digitalWrite(led_pin, LOW);
    }
}

/*** read data ***/

void OpticalDensityLoggerComponent::readData() {

    if (state->beam == BEAM_OFF) {
      // beam state OFF --> no reading
      Serial.printlnf("WARNING: beam is permanently OFF at %lu, cannot read data, turn to AUTO", millis());
      data_read_status = DATA_READ_COMPLETE;
    } else if (state->beam == BEAM_PAUSE) {
      // beam state PAUSEd --> no reading
      Serial.printlnf("WARNING: beam is temporarily OFF at %lu, cannot read data, turn to AUTO", millis());
      data_read_status = DATA_READ_COMPLETE;
    } else if (!zeroing && !state->is_zeroed) {
      // not zeroed yet
      Serial.printlnf("WARNING: not zeroed yet, skipping read at %lu", millis());
      data_read_status = DATA_READ_COMPLETE;
    } else if (beam_read_status == BEAM_READ_IDLE) {
      // idle -> start data
      startData();
      data_received_last = millis();
      // stop stirrer if it's on and 
      if (stirrer && stirrer->state->status == STATUS_ON) {
        stirrer->stepper.setSpeed(0); // stepper off
        stirrer_temp_off = true;
      }
      //  -> start dark beam or skip if beam state ON
      if (state->beam == BEAM_AUTO) {
        if (debug_component) Serial.printlnf("DEBUG: %lu - starting dark read", millis());
        updateBeam(BEAM_OFF);
        beam_read_status = BEAM_READ_WAIT_DARK;
      } else if (state->beam == BEAM_ON) {
        Serial.printlnf("WARNING: beam is permanently ON, skipping dark read and warmup wait");
        beam_read_status = BEAM_READ_BEAM;
      }
    } else if (beam_read_status == BEAM_READ_WAIT_DARK) {
      // wait for potential beam cooldown to finish
      if ((millis() - data_received_last) > state->warmup) {
        if (debug_component) Serial.printlnf("DEBUG: %lu - finished cooldown, starting signal read", millis());
        beam_read_status = BEAM_READ_DARK;
        data_received_last = millis();
      }
    } else if (beam_read_status == BEAM_READ_DARK) {
      // dark read --> read or if down move on to wait
      if ((millis() - data_received_last) > state->read_length) {
        if (debug_component) Serial.printlnf("DEBUG: %lu - finished dark read, turning beam on", millis());
        updateBeam(BEAM_ON);
        beam_read_status = BEAM_READ_WAIT_BEAM;
        data_received_last = millis();
      } else {
        collectDarkData();
      }
    } else if (beam_read_status == BEAM_READ_WAIT_BEAM) {
      // wait for beam warmup to finish
      if ((millis() - data_received_last) > state->warmup) {
        if (debug_component) Serial.printlnf("DEBUG: %lu - finished warmup, starting signal read", millis());
        beam_read_status = BEAM_READ_BEAM;
        data_received_last = millis();
      }
    } else if (beam_read_status == BEAM_READ_BEAM) {
      // read signal
      // FIXME: should this really be twice the read length?
      if ((millis() - data_received_last) > (2 * state->read_length)) {
        if (debug_component) Serial.printlnf("DEBUG: %lu - finished signal read", millis());
        if (state->beam == BEAM_AUTO) updateBeam(BEAM_OFF);
        data_read_status = DATA_READ_COMPLETE;
        beam_read_status = BEAM_READ_IDLE;
        // turn stirrer back on if done
        if (stirrer && stirrer_temp_off && (!zeroing || zero_read_counter + 1 > zero_read_n)) {
          stirrer->stepper.setSpeed(stirrer->calculateSpeed()); // stepper back on
          stirrer_temp_off = false;
        }
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

  // FIXME: implement warmup time BEFORE the dark read just to make sure we're not having cooldown issues
  // FIXME: deal with BEAM ON mode!!
  // --> show instead of OD: ??/ show 'beam: 3590' and refresh continuously (i.e. read after read after read) but don't story any of it in the actual data objects
  // --> i.e. require switch to auto mode for actual OD reading
  // FIXME: deal with saturation of sensor during zero-ing!!
  // FIXME: implement "beam pause" to pause OD readings (beam off) but not reset the zero (unlike beam OFF) and then resume with "beam auto"
  if (zeroing) {
    ref_zero_dark.add(ref_dark.getMean());
    sig_zero_dark.add(sig_dark.getMean());
    ref_zero.add(ref_beam.getMean());
    sig_zero.add(sig_beam.getMean());
    ratio_zero.add((sig_beam.getMean() - sig_dark.getMean()) / (ref_beam.getMean() - ref_dark.getMean()));
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
    data[7].setNewestValue(ref_dark.getMean()); data[7].saveNewestValue(true); // ref beam bgrd
    data[6].setNewestValue(sig_beam.getMean()); data[6].saveNewestValue(true); // ref beam value
    data[5].setNewestValue(sig_dark.getMean()); data[5].saveNewestValue(true); // sig beam bgrd
    data[4].setNewestValue(sig_beam.getMean()); data[4].saveNewestValue(true); // sig beam value
    data[3].saveRunningStatsValue(state->ratio_zero); // sig/ref zero value - this is a good idea to include, otherwise measurements can't be reconstructed
    float ratio = (sig_beam.getMean() - sig_dark.getMean()) / (ref_beam.getMean() - ref_dark.getMean());
    data[2].setNewestValue(ratio); data[2].saveNewestValue(true); // sig/ref beam value
    float transmittance = ratio / state->ratio_zero.getMean();
    data[1].setNewestValue(transmittance); data[1].saveNewestValue(true); // transmittance
    data[0].setNewestValue(-log10(transmittance)); data[0].saveNewestValue(true); // absorbance
  }
}


/*** logger state variable ***/

void OpticalDensityLoggerComponent::assembleStateVariable() {
  char pair[80];
  getOpticalDensityStateBeamInfo(state->beam, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getOpticalDensityStateZeroedInfo(state->is_zeroed, state->last_zero_datetime, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  // FIXME: use NAs (or NuLL?) when 
  getStateDoubleText("zero-ratio", state->ratio_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 4, true); ctrl->addToStateVariableBuffer(pair);
  getStateDoubleText("zero-beam-signal", state->sig_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 1, true); ctrl->addToStateVariableBuffer(pair);
  getStateDoubleText("zero-beam-bgrd", state->sig_zero_dark.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 1, true); ctrl->addToStateVariableBuffer(pair);
  getStateDoubleText("zero-ref-signal", state->ref_zero.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 1, true); ctrl->addToStateVariableBuffer(pair);
  getStateDoubleText("zero-ref-bgrd", state->ref_zero_dark.getMean(), "", pair, sizeof(pair), PATTERN_KV_JSON_QUOTED, 1, true); ctrl->addToStateVariableBuffer(pair);
}
