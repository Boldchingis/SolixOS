# Browser Makefile

# Source files
SOURCES = lynx.c
OBJECTS = $(SOURCES:.c=.o)

# Build rules
all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o

.PHONY: all clean
