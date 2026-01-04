/*
 * NERD HTTP Runtime - libcurl wrapper with JSON support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "nerd_json.h"

// Response buffer
typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

// Write callback for curl
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
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

// HTTP GET - returns response body as string
char* nerd_http_get(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
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

// HTTP POST - returns response body as string
char* nerd_http_post(const char *url, const char *body) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer buf = {0};
    buf.data = malloc(1);
    buf.size = 0;

    // Set Content-Type to JSON if body looks like JSON
    struct curl_slist *headers = NULL;
    if (body && (body[0] == '{' || body[0] == '[')) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
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

// Free response
void nerd_http_free(char *response) {
    free(response);
}

// HTTP GET - returns response as parsed JSON
nerd_json* nerd_http_get_json(const char *url) {
    char *response = nerd_http_get(url);
    if (!response) return NULL;
    
    nerd_json *json = nerd_json_parse(response);
    free(response);
    return json;
}

// HTTP POST - returns response as parsed JSON
nerd_json* nerd_http_post_json(const char *url, const char *body) {
    char *response = nerd_http_post(url, body);
    if (!response) return NULL;
    
    nerd_json *json = nerd_json_parse(response);
    free(response);
    return json;
}

// HTTP POST with JSON object body - stringify and send
nerd_json* nerd_http_post_json_body(const char *url, nerd_json *body) {
    char *body_str = nerd_json_stringify(body);
    if (!body_str) return NULL;
    
    nerd_json *result = nerd_http_post_json(url, body_str);
    free(body_str);
    return result;
}
