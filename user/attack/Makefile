CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L -D_DEFAULT_SOURCE -std=c11 -Wall -O1
LDFLAGS = -lssl -lcrypto -laircrack-osdep
SOURCES = dragontime.c utils.c
TARGET = dragontime

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)