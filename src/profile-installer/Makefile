CC=clang
# Build an executable suitable for debugging.
CFLAGS=-O0 -g

OBJS=parser.o main.o
TARGET=main.out

# Marking targets as phony tells make that they don't output a file and that they
# should be run on every build.
.PHONY: all clean

all: $(TARGET)

$(TARGET):	$(OBJS)
		$(CC) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)
