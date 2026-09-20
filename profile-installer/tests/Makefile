CC := clang
# Build an executable suitable for debugging.
CFLAGS := -O0 -g

BUILD_DIR := build
UNITY_HEADERS := vendor/unity.h vendor/unity_internals.h

.PHONY: all test clean

all: $(BUILD_DIR)/parser-suite.out $(BUILD_DIR)/db-interface-suite.out $(BUILD_DIR)/lib-suite.out

# --- shared by all test suites ---
$(BUILD_DIR)/unity.o: vendor/unity.c $(UNITY_HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c vendor/unity.c -o $(BUILD_DIR)/unity.o


# --- parser suite ---
PARSER_OBJS := $(addprefix $(BUILD_DIR)/, parser-suite.o parser.o unity.o)

$(BUILD_DIR)/parser-suite.out:  $(PARSER_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/parser-suite.out $(PARSER_OBJS)
$(BUILD_DIR)/parser-suite.o: parser-suite.c ../src/parser.h $(UNITY_HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c parser-suite.c -o $(BUILD_DIR)/parser-suite.o
$(BUILD_DIR)/parser.o: ../src/parser.c ../src/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../src/parser.c -o $(BUILD_DIR)/parser.o


## --- db interface suite ---
DB_INTERFACE_OBJS := $(addprefix $(BUILD_DIR)/, db-interface-suite.o unity.o sqlite3.o)

$(BUILD_DIR)/db-interface-suite.out:  $(DB_INTERFACE_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/db-interface-suite.out $(DB_INTERFACE_OBJS)
$(BUILD_DIR)/db-interface-suite.o: db-interface-suite.c $(UNITY_HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c db-interface-suite.c -o $(BUILD_DIR)/db-interface-suite.o
$(BUILD_DIR)/sqlite3.o: ../src/vendor/sqlite3/sqlite3.h | $(BUILD_DIR)
	$(CC) -O0 -c ../src/vendor/sqlite3/sqlite3.c -o $(BUILD_DIR)/sqlite3.o


# --- lib suite ---
LIB_OBJS := $(addprefix $(BUILD_DIR)/, lib-suite.o lib.o unity.o)

$(BUILD_DIR)/lib-suite.out:  $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/lib-suite.out $(LIB_OBJS)
$(BUILD_DIR)/lib-suite.o: lib-suite.c ../src/lib.h $(UNITY_HEADERS) 
	$(CC) $(CFLAGS) -c lib-suite.c -o $(BUILD_DIR)/lib-suite.o
$(BUILD_DIR)/lib.o: ../src/lib.c ../src/lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../src/lib.c -o $(BUILD_DIR)/lib.o


# -----------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

test:
	$(BUILD_DIR)/parser-suite.out
	$(BUILD_DIR)/db-interface-suite.out
	$(BUILD_DIR)/lib-suite.out

