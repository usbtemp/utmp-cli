OPTS := -O2 -s -std=c99 -I./include
CFLAGS += -Wall -Werror -Wextra -fdata-sections -ffunction-sections -fno-strict-aliasing
LDFLAGS += -Wl,--gc-sections
SFLAGS := -R .comment -R .gnu.version -R .note -R .note.ABI-tag

CC = $(CROSS_COMPILE)cc
STRIP = $(CROSS_COMPILE)strip

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))
ifeq ($(OS),Windows_NT)
	EXT := .exe
endif
TARGET := utmp-cli$(EXT)

all: $(OBJECTS) $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $(SFLAGS) $@

clean:
	rm -fr $(OBJECTS)
	rm -f $(TARGET)
