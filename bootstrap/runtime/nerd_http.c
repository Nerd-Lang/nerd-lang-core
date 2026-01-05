/*
 * NERD HTTP Runtime - Full HTTP support with libcurl
 * 
 * Features:
 * - All HTTP methods: GET, POST, PUT, DELETE, PATCH
 * - Custom headers
 * - Auth shortcuts: Bearer, Basic
 * - Response object: status, headers, body
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "nerd_json.h"

// Response buffer for body
typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

// Response buffer for headers
typedef struct {
    nerd_json *headers;
} HeaderBuffer;

// Write callback for response body
static size_t write_body_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    char *ptr = realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) return 0;

    buf->data = ptr;
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0;

    return realsize;
}

// Write callback for response headers
static size_t write_header_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    HeaderBuffer *buf = (HeaderBuffer *)userp;
    
    char *line = (char *)contents;
    
    // Find the colon separator
    char *colon = strchr(line, ':');
    if (colon && buf->headers) {
        // Extract header name
        size_t name_len = colon - line;
        char *name = malloc(name_len + 1);
        if (name) {
            strncpy(name, line, name_len);
            name[name_len] = '\0';
            
            // Extract header value (skip ": " and trim trailing \r\n)
            char *value_start = colon + 1;
            while (*value_start == ' ') value_start++;
            
            size_t value_len = realsize - (value_start - line);
            while (value_len > 0 && (value_start[value_len-1] == '\r' || value_start[value_len-1] == '\n')) {
                value_len--;
            }
            
            char *value = malloc(value_len + 1);
            if (value) {
                strncpy(value, value_start, value_len);
                value[value_len] = '\0';
                
                nerd_json_set_string(buf->headers, name, value);
                free(value);
            }
            free(name);
        }
    }
    
    return realsize;
}

// Base64 encoding for Basic auth
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* base64_encode(const char *input) {
    size_t len = strlen(input);
    size_t output_len = 4 * ((len + 2) / 3);
    char *output = malloc(output_len + 1);
    if (!output) return NULL;
    
    size_t i, j;
    for (i = 0, j = 0; i < len;) {
        uint32_t octet_a = i < len ? (unsigned char)input[i++] : 0;
        uint32_t octet_b = i < len ? (unsigned char)input[i++] : 0;
        uint32_t octet_c = i < len ? (unsigned char)input[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        output[j++] = base64_table[(triple >> 18) & 0x3F];
        output[j++] = base64_table[(triple >> 12) & 0x3F];
        output[j++] = base64_table[(triple >> 6) & 0x3F];
        output[j++] = base64_table[triple & 0x3F];
    }
    
    // Add padding
    size_t mod = len % 3;
    if (mod == 1) {
        output[output_len - 1] = '=';
        output[output_len - 2] = '=';
    } else if (mod == 2) {
        output[output_len - 1] = '=';
    }
    
    output[output_len] = '\0';
    return output;
}

/*
 * Core HTTP request function
 * Returns JSON object: { "status": 200, "headers": {...}, "body": ... }
 */
nerd_json* nerd_http_request(const char *method, const char *url, const char *body, nerd_json *headers) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer body_buf = {0};
    body_buf.data = malloc(1);
    body_buf.data[0] = '\0';
    body_buf.size = 0;

    HeaderBuffer header_buf = {0};
    header_buf.headers = nerd_json_new();

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set method
    if (strcmp(method, "GET") == 0) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "PATCH") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    }
    
    // Set body for methods that support it
    if (body && (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0)) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }
    
    // Build custom headers
    struct curl_slist *curl_headers = NULL;
    curl_headers = curl_slist_append(curl_headers, "User-Agent: nerd-lang/1.0");
    
    // Auto-detect JSON content type for body
    if (body && (body[0] == '{' || body[0] == '[')) {
        curl_headers = curl_slist_append(curl_headers, "Content-Type: application/json");
    }
    
    // Add custom headers from nerd_json object
    if (headers) {
        // Iterate through all keys in the headers object
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, headers) {
            if (item->string && cJSON_IsString(item)) {
                char header_line[2048];
                snprintf(header_line, sizeof(header_line), "%s: %s", item->string, item->valuestring);
                curl_headers = curl_slist_append(curl_headers, header_line);
            }
        }
    }
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    
    // Set callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body_buf);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&header_buf);
    
    // Other options
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    // Get status code
    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    curl_slist_free_all(curl_headers);
    curl_easy_cleanup(curl);

    // Build response object
    nerd_json *response = nerd_json_new();
    
    if (res != CURLE_OK) {
        nerd_json_set_number(response, "status", 0);
        nerd_json_set_string(response, "error", curl_easy_strerror(res));
        free(body_buf.data);
        nerd_json_free(header_buf.headers);
        return response;
    }
    
    // Set status
    nerd_json_set_number(response, "status", (double)status_code);
    
    // Set headers
    nerd_json_set_object(response, "headers", header_buf.headers);
    
    // Parse body as JSON if possible, otherwise set as string
    nerd_json *body_json = nerd_json_parse(body_buf.data);
    if (body_json) {
        nerd_json_set_object(response, "body", body_json);
    } else {
        nerd_json_set_string(response, "body", body_buf.data);
    }
    
    free(body_buf.data);
    return response;
}

/*
 * Convenience functions for each HTTP method
 */

// HTTP GET
nerd_json* nerd_http_get_full(const char *url, nerd_json *headers) {
    return nerd_http_request("GET", url, NULL, headers);
}

// HTTP POST
nerd_json* nerd_http_post_full(const char *url, const char *body, nerd_json *headers) {
    return nerd_http_request("POST", url, body, headers);
}

// HTTP PUT
nerd_json* nerd_http_put(const char *url, const char *body, nerd_json *headers) {
    return nerd_http_request("PUT", url, body, headers);
}

// HTTP DELETE
nerd_json* nerd_http_delete(const char *url, nerd_json *headers) {
    return nerd_http_request("DELETE", url, NULL, headers);
}

// HTTP PATCH
nerd_json* nerd_http_patch(const char *url, const char *body, nerd_json *headers) {
    return nerd_http_request("PATCH", url, body, headers);
}

/*
 * Auth helper functions
 */

// Create Bearer auth header
nerd_json* nerd_http_auth_bearer(const char *token) {
    nerd_json *headers = nerd_json_new();
    char auth_value[2048];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", token);
    nerd_json_set_string(headers, "Authorization", auth_value);
    return headers;
}

// Create Basic auth header
nerd_json* nerd_http_auth_basic(const char *username, const char *password) {
    nerd_json *headers = nerd_json_new();
    
    // Build "username:password" string
    size_t cred_len = strlen(username) + 1 + strlen(password);
    char *credentials = malloc(cred_len + 1);
    if (credentials) {
        snprintf(credentials, cred_len + 1, "%s:%s", username, password);
        
        // Base64 encode
        char *encoded = base64_encode(credentials);
        if (encoded) {
            char auth_value[2048];
            snprintf(auth_value, sizeof(auth_value), "Basic %s", encoded);
            nerd_json_set_string(headers, "Authorization", auth_value);
            free(encoded);
        }
        free(credentials);
    }
    
    return headers;
}

/*
 * Legacy functions for backwards compatibility
 * These return just the body for existing code
 */

// HTTP GET - returns response body as string (legacy)
char* nerd_http_get(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nerd-lang/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }

    return buf.data;
}

// HTTP POST - returns response body as string (legacy)
char* nerd_http_post(const char *url, const char *body) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    struct curl_slist *headers = NULL;
    if (body && (body[0] == '{' || body[0] == '[')) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nerd-lang/1.0");
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(buf.data);
        return NULL;
    }

    return buf.data;
}

// Free response (legacy)
void nerd_http_free(char *response) {
    free(response);
}

// HTTP GET - returns response as parsed JSON (legacy)
nerd_json* nerd_http_get_json(const char *url) {
    char *response = nerd_http_get(url);
    if (!response) return NULL;
    
    nerd_json *json = nerd_json_parse(response);
    free(response);
    return json;
}

// HTTP POST - returns response as parsed JSON (legacy)
nerd_json* nerd_http_post_json(const char *url, const char *body) {
    char *response = nerd_http_post(url, body);
    if (!response) return NULL;
    
    nerd_json *json = nerd_json_parse(response);
    free(response);
    return json;
}

// HTTP POST with JSON object body (legacy)
nerd_json* nerd_http_post_json_body(const char *url, nerd_json *body) {
    char *body_str = nerd_json_stringify(body);
    if (!body_str) return NULL;
    
    nerd_json *result = nerd_http_post_json(url, body_str);
    free(body_str);
    return result;
}
