/*
 * NERD MCP Runtime - Model Context Protocol support
 * 
 * Implements JSON-RPC 2.0 over HTTP for remote MCP servers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Structure to hold response data
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function for writing received data
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "MCP: out of memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Internal: Make a JSON-RPC POST request
static char* mcp_post(const char* url, const char* json_body) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    struct curl_slist *headers = NULL;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "nerd-mcp/1.0");
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

        // MCP requires specific headers
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json, text/event-stream");
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, json_body);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(json_body));

        res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK) {
            fprintf(stderr, "MCP request failed: %s\n", curl_easy_strerror(res));
            if (chunk.memory) free(chunk.memory);
            chunk.memory = NULL;
        }

        curl_easy_cleanup(curl_handle);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return chunk.memory;
}

// List available tools from an MCP server
// Returns JSON response (caller must free)
char* nerd_mcp_list(const char* url) {
    // JSON-RPC request for tools/list
    const char* request = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/list\",\"id\":1}";
    
    char* response = mcp_post(url, request);
    
    if (response) {
        printf("%s\n", response);
    }
    
    return response;
}

// Call a tool on an MCP server
// Returns JSON response (caller must free)
char* nerd_mcp_send(const char* url, const char* tool_name, const char* args_json) {
    // Build JSON-RPC request for tools/call
    // Format: {"jsonrpc":"2.0","method":"tools/call","params":{"name":"...","arguments":{...}},"id":2}
    
    size_t request_size = 256 + strlen(tool_name) + strlen(args_json);
    char* request = malloc(request_size);
    if (!request) {
        fprintf(stderr, "MCP: failed to allocate request buffer\n");
        return NULL;
    }
    
    snprintf(request, request_size,
        "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"params\":{\"name\":\"%s\",\"arguments\":%s},\"id\":2}",
        tool_name, args_json);
    
    char* response = mcp_post(url, request);
    free(request);
    
    if (response) {
        printf("%s\n", response);
    }
    
    return response;
}

// Initialize an MCP session (optional for some servers)
char* nerd_mcp_init(const char* url) {
    // JSON-RPC request for initialize
    const char* request = "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{},\"clientInfo\":{\"name\":\"nerd\",\"version\":\"0.1.0\"}},\"id\":0}";
    
    char* response = mcp_post(url, request);
    
    if (response) {
        printf("%s\n", response);
    }
    
    return response;
}

// Free memory allocated by MCP functions
void nerd_mcp_free(char* ptr) {
    if (ptr) {
        free(ptr);
    }
}

