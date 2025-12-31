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

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

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

# Compile and link to native executable (requires clang/LLVM)
native: $(BIN)
	./$(BIN) compile ../examples/math.nerd -o math.ll
	clang math.ll -o math_native -lm
	@echo "Built native executable: math_native"

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
