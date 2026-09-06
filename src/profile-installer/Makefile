CC=clang
# Build an executable suitable for debugging.
CFLAGS=-O0 -g

BUILD_DIR := build
OUT := main.out
OBJS := $(addprefix $(BUILD_DIR)/, parser.o main.o)
TARGET := $(addprefix $(BUILD_DIR)/, $(OUT))

# Marking targets as phony tells make that they don't output a file and that they
# should be run on every build.
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/main.o: main.c parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/parser.o: parser.c parser.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c parser.c -o $(BUILD_DIR)/parser.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(OBJS) $(TARGET)
