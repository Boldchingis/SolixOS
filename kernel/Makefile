# Kernel Makefile

# Source files
SOURCES = kernel.c mm.c interrupts.c
OBJECTS = $(SOURCES:.c=.o)

# Build rules
all: $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o

.PHONY: all clean
