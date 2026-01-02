/*
 * NERD LLM Runtime - Claude, OpenAI, etc.
 * 
 * Simple LLM calls without the boilerplate.
 * API keys from: 1) environment variables, or 2) .env file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

// Load .env file if it exists
static void load_env_file(void) {
    static int loaded = 0;
    if (loaded) return;
    loaded = 1;

    FILE *f = fopen(".env", "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        // Find = separator
        char *eq = strchr(line, '=');
        if (!eq) continue;

        // Extract key and value
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        // Trim newline from value
        size_t len = strlen(value);
        if (len > 0 && value[len-1] == '\n') value[len-1] = '\0';

        // Remove quotes if present
        if (value[0] == '"' && strlen(value) > 1) {
            value++;
            len = strlen(value);
            if (len > 0 && value[len-1] == '"') value[len-1] = '\0';
        }

        // Set environment variable (don't override existing)
        setenv(key, value, 0);
    }

    fclose(f);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Extract text content from Claude response
// Looks for: "text":"..." in the response
static char* extract_text(const char* json) {
    const char* text_start = strstr(json, "\"text\":\"");
    if (!text_start) return NULL;
    
    text_start += 8;  // Skip "text":"
    
    // Find the end of the text value
    const char* text_end = text_start;
    while (*text_end && !(*text_end == '"' && *(text_end - 1) != '\\')) {
        text_end++;
    }
    
    size_t len = text_end - text_start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    // Copy and unescape basic sequences
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (text_start[i] == '\\' && i + 1 < len) {
            char next = text_start[i + 1];
            if (next == 'n') { result[j++] = '\n'; i++; }
            else if (next == 't') { result[j++] = '\t'; i++; }
            else if (next == '"') { result[j++] = '"'; i++; }
            else if (next == '\\') { result[j++] = '\\'; i++; }
            else { result[j++] = text_start[i]; }
        } else {
            result[j++] = text_start[i];
        }
    }
    result[j] = '\0';
    
    return result;
}

// Call Claude (Anthropic)
// Returns extracted text response (caller must free)
char* nerd_llm_claude(const char* prompt) {
    // Try to load .env file
    load_env_file();

    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: ANTHROPIC_API_KEY not set\n");
        fprintf(stderr, "Set it via: export ANTHROPIC_API_KEY=... or in .env file\n");
        return NULL;
    }

    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    struct curl_slist *headers = NULL;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        // Build request body
        size_t body_size = strlen(prompt) + 512;
        char* body = malloc(body_size);
        snprintf(body, body_size,
            "{\"model\":\"claude-sonnet-4-20250514\",\"max_tokens\":1024,\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]}",
            prompt);

        // Set headers
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.anthropic.com/v1/messages");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "LLM request failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }

        free(body);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();

    // Extract and print the text
    if (chunk.memory) {
        char* text = extract_text(chunk.memory);
        if (text) {
            printf("%s\n", text);
            free(chunk.memory);
            return text;
        } else {
            // Print raw response if extraction fails
            printf("%s\n", chunk.memory);
        }
    }

    return chunk.memory;
}

void nerd_llm_free(char* ptr) {
    if (ptr) free(ptr);
}

