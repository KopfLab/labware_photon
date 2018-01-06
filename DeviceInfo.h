#pragma once
/**** helper functions for textual translations of state values ****/

// NOTE: size is always passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)
// NOTE: consider implementing better error catching for overlong key/value pairs

// formatting patterns
#define PATTERN_KVU_SIMPLE        "%s: %s%s"
#define PATTERN_KVU_JSON          "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\"}"
#define PATTERN_KVU_JSON_QUOTED   "{\"k\":\"%s\",\"v\":\"%s\",\"u\":\"%s\"}"
#define PATTERN_KV_SIMPLE         "%s: %s"
#define PATTERN_KV_JSON           "{\"k\":\"%s\",\"v\":%s}"
#define PATTERN_KV_JSON_QUOTED    "{\"k\":\"%s\",\"v\":\"%s\"}"
#define PATTERN_VU_SIMPLE         "%s%s"
#define PATTERN_V_SIMPLE          "%s"

// utility functions
static void getStateKeyValueUnits(char* target, int size, char* key, char* value, char* units, char* pattern = PATTERN_KVU_SIMPLE) {
  snprintf(target, size, pattern, key, value, units);
}

static void getStateKeyValue(char* target, int size, char* key, char* value, char* pattern = PATTERN_KV_SIMPLE) {
  snprintf(target, size, pattern, key, value);
}

static void getStateValueUnits(char* target, int size, char* value, char* units, char* pattern = PATTERN_VU_SIMPLE) {
  snprintf(target, size, pattern, value, units);
}

static void getStateValue(char* target, int size, char* value, char* pattern = PATTERN_V_SIMPLE) {
  snprintf(target, size, pattern, value);
}

// helper function to assemble boolean state text
static void getStateBooleanText(char* key, bool value, char* value_true, char* value_false, char* target, int size, char* pattern, bool include_key = true) {
  char value_text[20];
  value_text[sizeof(value_text) - 1] = 0; // make sure last index is null pointer just to be extra safe
  value ? strncpy(value_text, value_true, sizeof(value_text) - 1) : strncpy(value_text, value_false, sizeof(value_text) - 1);
  if (include_key)
    getStateKeyValue(target, size, key, value_text, pattern);
  else
    getStateValue(target, size, value_text, pattern);
}

// helper function to assemble integer state text
static void getStateIntText(char* key, int value, char* units, char* target, int size, char* pattern, bool include_key = true) {
  char value_text[10];
  snprintf(value_text, sizeof(value_text), "%d", value);
  if (include_key)
    getStateKeyValueUnits(target, size, key, value_text, units, pattern);
  else
    getStateValueUnits(target, size, value_text, units, pattern);
}
