# jack-midi-timing

CC ?= gcc

CFLAGS += -O0 -g -Wall -Wextra -std=gnu99
CFLAGS += $(shell pkg-config --cflags jack)

LDFLAGS += $(shell pkg-config --libs jack)

all: receiver sender

receiver: receiver.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

sender: sender.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f receiver sender
