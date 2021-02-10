#include "application.h"
#include "ScaleLoggerComponent.h"

/*** setup ***/

uint8_t ScaleLoggerComponent::setupDataVector(uint8_t start_idx) { 

    // resize data vector
    data.resize(2);

    // add data: idx, key
    data[0] = LoggerData(1, "weight");
    data[1] = LoggerData(2, "rate");
    // rate is persistent (i.e. not cleared after each log since it's calculated from two weights)
    data[1].makePersistent();

    return(start_idx + data.size()); 
}

/*** state management ***/
    
size_t ScaleLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void ScaleLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool ScaleLoggerComponent::restoreState() {
    ScaleState *saved_state = new ScaleState();
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

void ScaleLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}

/*** command parsing ***/

bool ScaleLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseCalcRate(command)) {
    // calc rate command parsed
  }
  return(command->isTypeDefined());
}

bool ScaleLoggerComponent::parseCalcRate(LoggerCommand *command) {
  if (command->parseVariable(CMD_CALC_RATE)) {
    // parse calc rate
    command->extractValue();
    if (command->parseValue(CMD_CALC_RATE_OFF)){
      // no rate calculation
      command->success(changeCalcRate(CALC_RATE_OFF));
    } else if (command->parseValue(CMD_CALC_RATE_SEC)) {
      // [mass]/second
      command->success(changeCalcRate(CALC_RATE_SEC));
    } else if (command->parseValue(CMD_CALC_RATE_MIN)) {
      // [mass]/minute
      command->success(changeCalcRate(CALC_RATE_MIN));
    } else if (command->parseValue(CMD_CALC_RATE_HR)) {
      // [mass]/hour
      command->success(changeCalcRate(CALC_RATE_HR));
    } else if (command->parseValue(CMD_CALC_RATE_DAY)) {
      // [mass]/day
      command->success(changeCalcRate(CALC_RATE_DAY));
    } else {
      // invalid value
      command->errorValue();
    }
    getStateCalcRateText(state->calc_rate, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

/*** state changes ***/

bool ScaleLoggerComponent::changeCalcRate(uint rate) {

  bool changed = rate != state->calc_rate;

  if (changed) {
    state->calc_rate = rate;
    calculateRate();
    ctrl->updateDataVariable();
  }
  
 (changed) ?
    Serial.printlnf("INFO: setting rate to %d", rate):
    Serial.printlnf("INFO: rate unchanged (%d)", rate);

  if (changed) saveState();

  return(changed);
}

/*** rate calculations ***/

void ScaleLoggerComponent::calculateRate() {

  if (state->calc_rate == CALC_RATE_OFF || prev_weight1.getN() == 0 || prev_weight2.getN() == 0) {
    // no rate calculation OR not enough data for rate calculation, make sure to clear
    data[1].clear(true);
  } else {
    // set rate units text
    char rate_units[10];
    strncpy(rate_units, data[0].units, sizeof(rate_units) - 1);
    strcpy(rate_units + strlen(data[0].units), "/");
    getStateCalcRateText(state->calc_rate, rate_units + strlen(data[0].units) + 1, sizeof(rate_units), true);
    rate_units[sizeof(rate_units) - 1] = 0; // safety
    data[1].setUnits(rate_units);
    
    // calculate rate
    double time_diff = (double) prev_data_time1 - (double) prev_data_time2;
    if (state->calc_rate == CALC_RATE_SEC) time_diff = time_diff / 1000.;
    else if (state->calc_rate == CALC_RATE_MIN) time_diff = time_diff / 1000. / 60.;
    else if (state->calc_rate == CALC_RATE_HR) time_diff = time_diff / 1000. / 60. / 60.;
    else if (state->calc_rate == CALC_RATE_DAY) time_diff = time_diff / 1000. / 60. / 60. / 24.;
    double rate = (prev_weight1.getMean() - prev_weight2.getMean()) / time_diff;
    data[1].setNewestValue(rate);

    // calculate mean data time
    unsigned long data_time = (unsigned long) round( 0.5 * (double) prev_data_time1 + 0.5 * (double) prev_data_time2 );
    data[1].setNewestDataTime(data_time);
    data[1].saveNewestValue(false);

    // n and variance
    data[1].value.n = prev_weight1.getN() + prev_weight2.getN();
    double variance = (prev_weight1.getVariance() + prev_weight2.getVariance()) / (time_diff * time_diff);
    data[1].value.M2 = variance * (data[1].value.n - 1); // a bit round-about but works

    // set decimals to 5 significant digits
    data[1].setDecimals(find_signif_decimals (rate, 5, false, 6));
  }
}

/*** manage data ***/

void ScaleLoggerComponent::finishData() {
    // weight
    if (error_counter == 0) {
        data[0].setNewestValue(value_buffer, true, 2L); // infer decimals and add 2 to improve accuracy of offline calculated rate
        data[0].saveNewestValue(true); // average
    }
}

/*** logger state variable ***/

void ScaleLoggerComponent::assembleStateVariable() {
    char pair[60];
    getStateCalcRateText(state->calc_rate, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** particle webhook data log ***/

void ScaleLoggerComponent::clearData(bool clear_persistent) {
  SerialReaderLoggerComponent::clearData(clear_persistent);
  if (clear_persistent) {
    // also reset stored weight data
    prev_weight1.clear();
    prev_weight2.clear();
  }
}

void ScaleLoggerComponent::logData() {
  prev_weight2 = prev_weight1;
  prev_data_time2 = prev_data_time1;
  prev_weight1 = data[0].value;
  prev_data_time1 = data[0].data_time.getMean();
  calculateRate();
  SerialReaderLoggerComponent::logData();
}