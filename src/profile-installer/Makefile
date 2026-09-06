CC=clang
# Build an executable suitable for debugging.
CFLAGS=-O0 -g

BUILD_DIR := build
TARGET := main.out
OBJS := $(addprefix $(BUILD_DIR)/, parser.o main.o)
OUT := $(addprefix $(BUILD_DIR)/, $(TARGET))

# Marking targets as phony tells make that they don't output a file and that they
# should be run on every build.
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $(OUT) $(OBJS)

$(BUILD_DIR)/main.o: main.c parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/parser.o: parser.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c parser.c -o $(BUILD_DIR)/parser.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean::
	rm -f $(OBJS) $(TARGET)
