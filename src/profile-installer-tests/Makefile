CC=clang
# Build an executable suitable for debugging.
CFLAGS=-O0 -g

BUILD_DIR := build
OUT := main.out
OBJS := $(addprefix $(BUILD_DIR)/, unity.o parser.o main.o)
TARGET := $(addprefix $(BUILD_DIR)/, $(OUT))

# Marking targets as phony tells make that they don't output a file and that they
# should be run on every build.
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/main.o: main.c vendor/unity.h vendor/unity_internals.h ../profile-installer/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o
$(BUILD_DIR)/unity.o: vendor/unity.c vendor/unity.h vendor/unity_internals.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c vendor/unity.c -o $(BUILD_DIR)/unity.o
$(BUILD_DIR)/parser.o: ../profile-installer/parser.c ../profile-installer/parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c ../profile-installer/parser.c -o $(BUILD_DIR)/parser.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(OBJS) $(OUT)
