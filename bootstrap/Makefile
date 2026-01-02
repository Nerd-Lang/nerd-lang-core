# NERD Bootstrap Compiler Makefile
# No dependencies except standard C library

CC = cc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I./include
LDFLAGS =

# Debug build
DEBUG_CFLAGS = -Wall -Wextra -std=c11 -g -O0 -I./include -DDEBUG

SRC_DIR = src
BUILD_DIR = build
BIN = nerd

# Exclude runtime files from compiler build
SOURCES = $(filter-out $(SRC_DIR)/nerd_http.c $(SRC_DIR)/nerd_mcp.c $(SRC_DIR)/nerd_llm.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Runtime libraries
RUNTIME_HTTP_SRC = $(SRC_DIR)/nerd_http.c
RUNTIME_HTTP_OBJ = $(BUILD_DIR)/nerd_http.o
RUNTIME_MCP_SRC = $(SRC_DIR)/nerd_mcp.c
RUNTIME_MCP_OBJ = $(BUILD_DIR)/nerd_mcp.o
RUNTIME_LLM_SRC = $(SRC_DIR)/nerd_llm.c
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

# Build HTTP runtime library
runtime: $(BUILD_DIR) $(RUNTIME_HTTP_OBJ)
	@echo "Built HTTP runtime: $(RUNTIME_HTTP_OBJ)"

$(RUNTIME_HTTP_OBJ): $(RUNTIME_HTTP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

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
	clang http_test.ll $(RUNTIME_HTTP_OBJ) -lcurl -o http_test
	@echo "Built HTTP-enabled executable: http_test"

# Compile with MCP support (requires libcurl)
native-mcp: $(BIN) runtime runtime-mcp
	./$(BIN) compile ../examples/mcp_test.nerd -o mcp_test.ll
	clang mcp_test.ll $(RUNTIME_HTTP_OBJ) $(RUNTIME_MCP_OBJ) -lcurl -o mcp_test
	@echo "Built MCP-enabled executable: mcp_test"

# Compile with full agent support (MCP + LLM)
native-agent: $(BIN) runtime-all
	./$(BIN) compile ../examples/agent.nerd -o agent.ll
	clang agent.ll $(RUNTIME_HTTP_OBJ) $(RUNTIME_MCP_OBJ) $(RUNTIME_LLM_OBJ) -lcurl -o agent
	@echo "Built agent executable: agent"

# Install to /usr/local/bin
install: $(BIN)
	cp $(BIN) /usr/local/bin/nerd
	@echo "Installed to /usr/local/bin/nerd"

# Help
help:
	@echo "NERD Bootstrap Compiler"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the compiler (default)"
	@echo "  debug   - Build with debug symbols"
	@echo "  clean   - Remove build artifacts"
	@echo "  test    - Run tests"
	@echo "  native  - Compile example to native (requires clang)"
	@echo "  install - Install to /usr/local/bin"
	@echo ""
	@echo "Usage after build:"
	@echo "  ./nerd compile <file.nerd> [-o output.ll]"
	@echo "  ./nerd parse <file.nerd>"
	@echo "  ./nerd tokens <file.nerd>"
