# NERD Bootstrap Compiler Makefile
# No dependencies except standard C library

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I./include
LDFLAGS =

# Debug build
DEBUG_CFLAGS = -Wall -Wextra -std=c11 -g -O0 -I./include -DDEBUG

# Directories
SRC_DIR = src
RUNTIME_DIR = runtime
LIB_DIR = lib
BUILD_DIR = build
BIN = nerd

# Compiler sources (only core compiler files)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Third-party libraries
LIB_CJSON_SRC = $(LIB_DIR)/cjson/cJSON.c
LIB_CJSON_OBJ = $(BUILD_DIR)/cJSON.o

# Runtime libraries
RUNTIME_JSON_SRC = $(RUNTIME_DIR)/nerd_json.c
RUNTIME_JSON_OBJ = $(BUILD_DIR)/nerd_json.o
RUNTIME_HTTP_SRC = $(RUNTIME_DIR)/nerd_http.c
RUNTIME_HTTP_OBJ = $(BUILD_DIR)/nerd_http.o
RUNTIME_MCP_SRC = $(RUNTIME_DIR)/nerd_mcp.c
RUNTIME_MCP_OBJ = $(BUILD_DIR)/nerd_mcp.o
RUNTIME_LLM_SRC = $(RUNTIME_DIR)/nerd_llm.c
RUNTIME_LLM_OBJ = $(BUILD_DIR)/nerd_llm.o

.PHONY: all clean debug test

all: $(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(BIN) *.o
	find . -maxdepth 1 -name "*.ll" ! -name "test_*.ll" -delete

# Test with examples
test: $(BIN)
	@echo "=== Testing NERD Compiler ==="
	@echo ""
	@echo "--- Tokenizing math.nerd ---"
	./$(BIN) tokens ../examples/math.nerd
	@echo ""
	@echo "--- Parsing math.nerd ---"
	./$(BIN) parse ../examples/math.nerd
	@echo ""
	@echo "--- Compiling math.nerd ---"
	./$(BIN) compile ../examples/math.nerd -o math.ll
	@echo ""
	@echo "--- Generated LLVM IR ---"
	@cat math.ll
	@echo ""
	@echo "=== Tests Complete ==="

# Build cJSON library (third-party)
$(LIB_CJSON_OBJ): $(LIB_CJSON_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build JSON runtime library (depends on cJSON)
runtime-json: $(BUILD_DIR) $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ)
	@echo "Built JSON runtime: $(RUNTIME_JSON_OBJ)"

$(RUNTIME_JSON_OBJ): $(RUNTIME_JSON_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(RUNTIME_DIR) -c -o $@ $<

# Build HTTP runtime library (depends on JSON)
runtime: $(BUILD_DIR) $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ) $(RUNTIME_HTTP_OBJ)
	@echo "Built HTTP runtime: $(RUNTIME_HTTP_OBJ)"

$(RUNTIME_HTTP_OBJ): $(RUNTIME_HTTP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(RUNTIME_DIR) -c -o $@ $<

# Build MCP runtime library
runtime-mcp: $(BUILD_DIR) $(RUNTIME_MCP_OBJ)
	@echo "Built MCP runtime: $(RUNTIME_MCP_OBJ)"

$(RUNTIME_MCP_OBJ): $(RUNTIME_MCP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build LLM runtime library
runtime-llm: $(BUILD_DIR) $(RUNTIME_LLM_OBJ)
	@echo "Built LLM runtime: $(RUNTIME_LLM_OBJ)"

$(RUNTIME_LLM_OBJ): $(RUNTIME_LLM_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build all runtimes
runtime-all: runtime runtime-mcp runtime-llm
	@echo "Built all runtime libraries"

# Compile and link to native executable (requires clang/LLVM)
native: $(BIN)
	./$(BIN) compile ../examples/math.nerd -o math.ll
	clang math.ll -o math_native -lm
	@echo "Built native executable: math_native"

# Compile with HTTP support (requires libcurl)
native-http: $(BIN) runtime
	./$(BIN) compile ../examples/http_test.nerd -o http_test.ll
	clang http_test.ll $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ) $(RUNTIME_HTTP_OBJ) -lcurl -o http_test
	@echo "Built HTTP-enabled executable: http_test"

# Compile with MCP support (requires libcurl)
native-mcp: $(BIN) runtime runtime-mcp
	./$(BIN) compile ../examples/mcp_test.nerd -o mcp_test.ll
	clang mcp_test.ll $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ) $(RUNTIME_HTTP_OBJ) $(RUNTIME_MCP_OBJ) -lcurl -o mcp_test
	@echo "Built MCP-enabled executable: mcp_test"

# Compile with full agent support (MCP + LLM)
native-agent: $(BIN) runtime-all
	./$(BIN) compile ../examples/agent.nerd -o agent.ll
	clang agent.ll $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ) $(RUNTIME_HTTP_OBJ) $(RUNTIME_MCP_OBJ) $(RUNTIME_LLM_OBJ) -lcurl -o agent
	@echo "Built agent executable: agent"

# Compile JSON test example
native-json: $(BIN) runtime
	./$(BIN) compile ../examples/json_test.nerd -o json_test.ll
	clang json_test.ll $(LIB_CJSON_OBJ) $(RUNTIME_JSON_OBJ) $(RUNTIME_HTTP_OBJ) -lcurl -o json_test
	@echo "Built JSON test executable: json_test"

# Install to /usr/local/bin
install: $(BIN)
	cp $(BIN) /usr/local/bin/nerd
	@echo "Installed to /usr/local/bin/nerd"

# Release packaging
DIST_DIR = dist
RELEASE_NAME = nerd-darwin-arm64

release: $(BIN) runtime-all
	@echo "Creating release package..."
	rm -rf $(DIST_DIR)
	mkdir -p $(DIST_DIR)/$(RELEASE_NAME)/lib
	cp $(BIN) $(DIST_DIR)/$(RELEASE_NAME)/
	cp $(LIB_CJSON_OBJ) $(DIST_DIR)/$(RELEASE_NAME)/lib/
	cp $(RUNTIME_JSON_OBJ) $(DIST_DIR)/$(RELEASE_NAME)/lib/
	cp $(RUNTIME_HTTP_OBJ) $(DIST_DIR)/$(RELEASE_NAME)/lib/
	cp $(RUNTIME_MCP_OBJ) $(DIST_DIR)/$(RELEASE_NAME)/lib/
	cp $(RUNTIME_LLM_OBJ) $(DIST_DIR)/$(RELEASE_NAME)/lib/
	cd $(DIST_DIR) && tar -czvf $(RELEASE_NAME).tar.gz $(RELEASE_NAME)
	@echo ""
	@echo "Release created: $(DIST_DIR)/$(RELEASE_NAME).tar.gz"
	@echo ""
	@echo "Contents:"
	@tar -tzvf $(DIST_DIR)/$(RELEASE_NAME).tar.gz
	@echo ""
	@echo "To install:"
	@echo "  tar -xzf $(RELEASE_NAME).tar.gz"
	@echo "  mv $(RELEASE_NAME) ~/.nerd"
	@echo "  export PATH=\$$PATH:~/.nerd/$(RELEASE_NAME)"

# Help
help:
	@echo "NERD Bootstrap Compiler"
	@echo ""
	@echo "Directory Structure:"
	@echo "  src/      - Compiler core (lexer, parser, codegen, main)"
	@echo "  runtime/  - Runtime libraries (http, json, mcp, llm)"
	@echo "  lib/      - Third-party libraries (cJSON)"
	@echo "  include/  - Public headers"
	@echo "  build/    - Compiled artifacts"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the compiler (default)"
	@echo "  runtime-all - Build all runtime libraries"
	@echo "  debug       - Build with debug symbols"
	@echo "  clean       - Remove build artifacts"
	@echo "  test        - Run tests"
	@echo "  native      - Compile example to native (requires clang)"
	@echo "  install     - Install to /usr/local/bin"
	@echo ""
	@echo "Usage after build:"
	@echo "  ./nerd compile <file.nerd> [-o output.ll]"
	@echo "  ./nerd run <file.nerd>"
	@echo "  ./nerd parse <file.nerd>"
	@echo "  ./nerd tokens <file.nerd>"
