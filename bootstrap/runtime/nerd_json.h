/*
 * NERD JSON Runtime - Path-based JSON access wrapper around cJSON
 */

#ifndef NERD_JSON_H
#define NERD_JSON_H

#include "../lib/cjson/cJSON.h"

// Use cJSON as our underlying type
typedef cJSON nerd_json;

// Parse/Create
nerd_json* nerd_json_parse(const char* str);
nerd_json* nerd_json_new(void);

// Access with path (supports "a.b.c" and "a[0].b" notation)
char* nerd_json_get_string(nerd_json* j, const char* path);
double nerd_json_get_number(nerd_json* j, const char* path);
int nerd_json_get_bool(nerd_json* j, const char* path);
nerd_json* nerd_json_get_object(nerd_json* j, const char* path);

// Utilities
int nerd_json_count(nerd_json* j, const char* path);  // Array length
int nerd_json_has(nerd_json* j, const char* path);    // Key exists

// Setters
void nerd_json_set_string(nerd_json* j, const char* key, const char* val);
void nerd_json_set_number(nerd_json* j, const char* key, double val);
void nerd_json_set_bool(nerd_json* j, const char* key, int val);
void nerd_json_set_object(nerd_json* j, const char* key, nerd_json* val);

// Serialize
char* nerd_json_stringify(nerd_json* j);

// Memory management
void nerd_json_free(nerd_json* j);
void nerd_json_free_string(char* str);

#endif

