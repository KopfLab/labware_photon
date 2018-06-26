#pragma once

#include <math.h>

/**** general parameters and constants for device cloude interaction ****/

// command from spark cloud
#define CMD_ROOT              "device" // command root (i.e. registered particle call function)
#define CMD_MAX_CHAR          63  // spark.functions are limited to 63 char long call

#define STATE_INFO_VARIABLE   "device_state" // name of the particle exposed state variable
#define STATE_INFO_MAX_CHAR   600 // how long is the state information maximally

#define STATE_LOG_WEBHOOK     "device_state_log"  // name of the webhook to device state log
#define STATE_LOG_MAX_CHAR    255  // spark.publish is limited to 255 chars of data

#define DATA_INFO_VARIABLE    "device_data" // name of the particle exposed data variable
#define DATA_INFO_MAX_CHAR    600 // how long is the data information maximally

#define DATA_LOG_WEBHOOK      "device_data_log"  // name of the webhook to device data log
#define DATA_LOG_MAX_CHAR     255  // spark.publish is limited to 255 chars of data


/**** helper functions for textual translations of state values ****/

// NOTE: size is always passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)
// NOTE: consider implementing better error catching for overlong key/value pairs

// formatting patterns
#define PATTERN_KVUNT_JSON        "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%lu,\"to\":%d}"

#define PATTERN_KVUN_SIMPLE      "%s: %s%s (%d)"
#define PATTERN_KVUN_JSON        "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%d}"

#define PATTERN_KVU_SIMPLE        "%s: %s%s"
#define PATTERN_KVU_JSON          "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\"}"
#define PATTERN_KVU_JSON_QUOTED   "{\"k\":\"%s\",\"v\":\"%s\",\"u\":\"%s\"}"

#define PATTERN_KV_SIMPLE         "%s: %s"
#define PATTERN_KV_JSON           "{\"k\":\"%s\",\"v\":%s}"
#define PATTERN_KV_JSON_QUOTED    "{\"k\":\"%s\",\"v\":\"%s\"}"

#define PATTERN_VUN_SIMPLE        "%s%s (%d)"
#define PATTERN_VU_SIMPLE         "%s%s"
#define PATTERN_V_SIMPLE          "%s"

/**** GENERAL UTILITY FUNCTIONS ****/

static void getInfoKeyValueUnitsNumberTimeOffset(char* target, int size, char* key, char* value, char* units, int n, unsigned long time_offset, char* pattern) {
  snprintf(target, size, pattern, key, value, units, n, time_offset);
}

static void getInfoKeyValueUnitsNumber(char* target, int size, char* key, char* value, char* units, int n, char* pattern = PATTERN_KVUN_SIMPLE) {
  snprintf(target, size, pattern, key, value, units, n);
}

static void getInfoValueUnitsNumber(char* target, int size, char* value, char* units, int n, char* pattern = PATTERN_VUN_SIMPLE) {
  snprintf(target, size, pattern, value, units, n);
}

static void getInfoKeyValueUnits(char* target, int size, char* key, char* value, char* units, char* pattern = PATTERN_KVU_SIMPLE) {
  snprintf(target, size, pattern, key, value, units);
}

static void getInfoKeyValue(char* target, int size, char* key, char* value, char* pattern = PATTERN_KV_SIMPLE) {
  snprintf(target, size, pattern, key, value);
}

static void getInfoValueUnits(char* target, int size, char* value, char* units, char* pattern = PATTERN_VU_SIMPLE) {
  snprintf(target, size, pattern, value, units);
}

static void getInfoValue(char* target, int size, char* value, char* pattern = PATTERN_V_SIMPLE) {
  snprintf(target, size, pattern, value);
}

/**** NUMERIC DATA FUNCTIONS ****/

// extrat number of decimals from a textual number representation
int find_number_of_decimals (char* text, const char* sep = ".") {
    int decimals = strlen(text) - strcspn(text, sep) - 1;
    if (decimals < 0) decimals = 0;
    return(decimals);
}

// the following functions are designed to make it easy to print to the significant digits of the error
// note that in these functions pos digits rounds to integers, neg. digits rounds to decimals
int find_first_digit (double value) {
    if (value == 0.0) return (-10); // what to do with this rare case? round to 10 digits
    else return(floor(log10(fabs(value)))); // could do this faster than with the log with a while if this is a problem
}

int find_rounding_digits (double value, double error, uint which = 1, bool decimals_only = false) {
    int error_digit = find_first_digit (error) - which + 1;
    int value_digit = find_first_digit (value) - which + 1;
    if (error_digit > value_digit) error_digit = value_digit;
    if (decimals_only && error_digit > 0) error_digit = 0;
    return(error_digit);
}

double round_to_digits(double value, int digits) {
    double factor = pow(10.0, -digits);
    return(round(value * factor) / factor);
}

void print_to_digits(char* target, int size, double value, int digits) {

    // round
    double rounded_value = round_to_digits(value, digits);

    // print
    digits = (digits > 0) ? 0 : -digits;
    char value_pattern[6];
    snprintf(value_pattern, sizeof(value_pattern), "%%.%df", digits);
    snprintf(target, size, value_pattern, rounded_value);
}


/**** DATA INFO FUNCTIONS ****/

static void getDataDoubleText(char* key, double value, char* units, int n, unsigned long time_offset, char* target, int size, char* pattern, int digits) {
  char value_text[20];
  print_to_digits(value_text, sizeof(value_text), value, digits);
  getInfoKeyValueUnitsNumberTimeOffset(target, size, key, value_text, units, n, time_offset, pattern);
}

static void getDataDoubleText(char* key, double value, char* units, int n, char* target, int size, char* pattern, int digits) {
  getDataDoubleText(key, value, units, n, -1, target, size, pattern, digits);
}

static void getDataDoubleText(char* key, double value, char* units, char* target, int size, char* pattern, int digits) {
  getDataDoubleText(key, value, units, -1, target, size, pattern, digits);
}

static void getDataNullText(char* key, char* target, int size, char* pattern) {
  char value_text[] = "null";
  getInfoKeyValue(target, size, key, value_text, pattern);
}
/**** STATE INFO FUNCTIONS ****/

// helper function to assemble char/string state text
static void getStateStringText(char* key, char* value, char* target, int size, char* pattern, bool include_key = true) {
  if (include_key)
    getInfoKeyValue(target, size, key, value, pattern);
  else
    getInfoValue(target, size, value, pattern);
}

// helper function to assemble boolean state text
static void getStateBooleanText(char* key, bool value, char* value_true, char* value_false, char* target, int size, char* pattern, bool include_key = true) {
  char value_text[20];
  value_text[sizeof(value_text) - 1] = 0; // make sure last index is null pointer just to be extra safe
  value ? strncpy(value_text, value_true, sizeof(value_text) - 1) : strncpy(value_text, value_false, sizeof(value_text) - 1);
  if (include_key)
    getInfoKeyValue(target, size, key, value_text, pattern);
  else
    getInfoValue(target, size, value_text, pattern);
}

// helper function to assemble integer state text
static void getStateIntText(char* key, int value, char* units, char* target, int size, char* pattern, bool include_key = true) {
  char value_text[10];
  snprintf(value_text, sizeof(value_text), "%d", value);
  if (include_key)
    getInfoKeyValueUnits(target, size, key, value_text, units, pattern);
  else
    getInfoValueUnits(target, size, value_text, units, pattern);
}
