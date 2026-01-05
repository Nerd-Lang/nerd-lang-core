/*
 * NERD JSON Runtime - Path-based JSON access wrapper around cJSON
 * 
 * Supports paths like:
 *   "name"              - Simple key
 *   "user.name"         - Nested object
 *   "items[0]"          - Array index
 *   "users[0].profile"  - Mixed access
 */

#include "nerd_json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Internal: Navigate to a JSON node using path notation
// Returns NULL if path not found
static cJSON* navigate_path(cJSON* root, const char* path) {
    if (!root || !path || !*path) {
        return root;
    }
    
    cJSON* current = root;
    char* path_copy = strdup(path);
    if (!path_copy) return NULL;  // Memory allocation failure
    char* ptr = path_copy;
    char* token_start = ptr;
    
    while (*ptr && current) {
        // Handle array index [n]
        if (*ptr == '[') {
            // First, get the object if there's a key before [
            if (ptr > token_start) {
                *ptr = '\0';
                current = cJSON_GetObjectItemCaseSensitive(current, token_start);
                if (!current) break;
            }
            
            // Parse array index
            ptr++; // skip [
            char* end_bracket = strchr(ptr, ']');
            if (!end_bracket) {
                current = NULL;
                break;
            }
            *end_bracket = '\0';
            
            int index = atoi(ptr);
            // Handle negative indices
            if (index < 0) {
                int len = cJSON_GetArraySize(current);
                index = len + index;
            }
            
            current = cJSON_GetArrayItem(current, index);
            ptr = end_bracket + 1;
            
            // Skip the dot after ]
            if (*ptr == '.') {
                ptr++;
            }
            token_start = ptr;
        }
        // Handle dot separator
        else if (*ptr == '.') {
            *ptr = '\0';
            if (ptr > token_start) {
                current = cJSON_GetObjectItemCaseSensitive(current, token_start);
            }
            ptr++;
            token_start = ptr;
        }
        else {
            ptr++;
        }
    }
    
    // Handle remaining token
    if (current && token_start && *token_start) {
        current = cJSON_GetObjectItemCaseSensitive(current, token_start);
    }
    
    free(path_copy);
    return current;
}

// Parse JSON string into object
nerd_json* nerd_json_parse(const char* str) {
    if (!str) return NULL;
    return cJSON_Parse(str);
}

// Create new empty JSON object
nerd_json* nerd_json_new(void) {
    return cJSON_CreateObject();
}

// Get string value at path
// Returns empty string if not found or not a string
char* nerd_json_get_string(nerd_json* j, const char* path) {
    cJSON* node = navigate_path(j, path);
    if (node && cJSON_IsString(node) && node->valuestring) {
        return strdup(node->valuestring);
    }
    return strdup("");  // Returns NULL on allocation failure, which is handled
}

// Get number value at path
// Returns 0 if not found or not a number
double nerd_json_get_number(nerd_json* j, const char* path) {
    cJSON* node = navigate_path(j, path);
    if (node && cJSON_IsNumber(node)) {
        return node->valuedouble;
    }
    return 0.0;
}

// Get boolean value at path
// Returns 0 (false) if not found
int nerd_json_get_bool(nerd_json* j, const char* path) {
    cJSON* node = navigate_path(j, path);
    if (node) {
        if (cJSON_IsTrue(node)) return 1;
        if (cJSON_IsFalse(node)) return 0;
    }
    return 0;
}

// Get object/array at path (for further navigation)
nerd_json* nerd_json_get_object(nerd_json* j, const char* path) {
    return navigate_path(j, path);
}

// Get array length at path
// Returns 0 if not an array
int nerd_json_count(nerd_json* j, const char* path) {
    cJSON* node = navigate_path(j, path);
    if (node && cJSON_IsArray(node)) {
        return cJSON_GetArraySize(node);
    }
    return 0;
}

// Check if key exists at path
int nerd_json_has(nerd_json* j, const char* path) {
    cJSON* node = navigate_path(j, path);
    return node != NULL ? 1 : 0;
}

// Set string value (only top-level keys for now)
void nerd_json_set_string(nerd_json* j, const char* key, const char* val) {
    if (!j || !key) return;
    
    // Remove existing key if present
    cJSON_DeleteItemFromObject(j, key);
    
    // Add new value
    cJSON_AddStringToObject(j, key, val ? val : "");
}

// Set number value
void nerd_json_set_number(nerd_json* j, const char* key, double val) {
    if (!j || !key) return;
    
    cJSON_DeleteItemFromObject(j, key);
    cJSON_AddNumberToObject(j, key, val);
}

// Set boolean value
void nerd_json_set_bool(nerd_json* j, const char* key, int val) {
    if (!j || !key) return;
    
    cJSON_DeleteItemFromObject(j, key);
    cJSON_AddBoolToObject(j, key, val);
}

// Set object value (takes ownership of val)
void nerd_json_set_object(nerd_json* j, const char* key, nerd_json* val) {
    if (!j || !key || !val) return;
    
    cJSON_DeleteItemFromObject(j, key);
    cJSON_AddItemToObject(j, key, val);
}

// Serialize to JSON string
// Caller must free with nerd_json_free_string
char* nerd_json_stringify(nerd_json* j) {
    if (!j) return strdup("{}");
    return cJSON_PrintUnformatted(j);
}

// Free JSON object
void nerd_json_free(nerd_json* j) {
    if (j) {
        cJSON_Delete(j);
    }
}

// Free string returned by stringify or get_string
void nerd_json_free_string(char* str) {
    if (str) {
        free(str);
    }
}

