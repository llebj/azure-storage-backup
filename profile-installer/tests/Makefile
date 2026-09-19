CC := clang
# Build an executable suitable for debugging.
CFLAGS := -O0 -g

BUILD_DIR := build
OUT := parser-suite.out
OBJS := $(addprefix $(BUILD_DIR)/, unity.o parser.o lib.o parser-suite.o)
TARGET := $(addprefix $(BUILD_DIR)/, $(OUT))

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/unity.o: vendor/unity.c vendor/unity.h vendor/unity_internals.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c vendor/unity.c -o $(BUILD_DIR)/unity.o

$(BUILD_DIR)/parser-suite.o: parser-suite.c vendor/unity.h vendor/unity_internals.h ../src/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c parser-suite.c -o $(BUILD_DIR)/parser-suite.o
$(BUILD_DIR)/parser.o: ../src/parser.c ../src/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../src/parser.c -o $(BUILD_DIR)/parser.o
$(BUILD_DIR)/lib.o: ../src/lib.c ../src/lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../src/lib.c -o $(BUILD_DIR)/lib.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

test:
	$(BUILD_DIR)/$(OUT)
