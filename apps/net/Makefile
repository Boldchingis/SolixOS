# Network Applications Makefile

# Source files
SOURCES = ping.c ifconfig.c curl.c
OBJECTS = $(SOURCES:.c=.o)

# Build rules
all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o

.PHONY: all clean
