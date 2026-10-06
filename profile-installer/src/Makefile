CC := clang
# Build an executable suitable for debugging.
CFLAGS := -O0 -g
LDLIBS := $(shell pkg-config --libs libsystemd)

BUILD_DIR := build
OUT := main.out
OBJS := $(addprefix $(BUILD_DIR)/, lib.o parser.o db-interface.o sqlite3.o main.o)
TARGET := $(addprefix $(BUILD_DIR)/, $(OUT))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $(TARGET) $(OBJS) $(LDLIBS)

$(BUILD_DIR)/main.o: main.c parser.h db-interface.h lib.h vendor/sqlite3/sqlite3.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/parser.o: parser.c parser.h lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c parser.c -o $(BUILD_DIR)/parser.o

$(BUILD_DIR)/db-interface.o: db-interface.h lib.h vendor/sqlite3/sqlite3.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c db-interface.c -o $(BUILD_DIR)/db-interface.o

LIB_CFLAGS := $(CFLAGS) $(shell pkg-config --cflags libsystemd)
$(BUILD_DIR)/lib.o: lib.c lib.h | $(BUILD_DIR)
	$(CC) $(LIB_CFLAGS) -c lib.c -o $(BUILD_DIR)/lib.o

$(BUILD_DIR)/sqlite3.o: vendor/sqlite3/sqlite3.c vendor/sqlite3/sqlite3.h | $(BUILD_DIR)
	$(CC) -O2 -c vendor/sqlite3/sqlite3.c -o $(BUILD_DIR)/sqlite3.o
	strip -x $(BUILD_DIR)/sqlite3.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
