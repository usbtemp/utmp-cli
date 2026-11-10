OPTS := -O2 -std=c99 -I./include
CFLAGS += -Wall -Werror -Wextra -fdata-sections -ffunction-sections -fno-strict-aliasing
LDFLAGS += -Wl,--gc-sections
comma := ,
SFLAGS := -R .comment -R .gnu.version -R .note -R .note.ABI-tag

CC = $(CROSS_COMPILE)cc
STRIP = $(CROSS_COMPILE)strip

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))
ifeq ($(OS),Windows_NT)
	EXT := .exe
endif
# Apple's linker and strip don't understand the GNU-only options.
ifeq ($(shell uname -s 2>/dev/null),Darwin)
	LDFLAGS := $(filter-out -Wl$(comma)--gc-sections,$(LDFLAGS)) -Wl,-dead_strip
	SFLAGS :=
endif
TARGET := utmp-cli$(EXT)

.PHONY: all clean

all: $(OBJECTS) $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(OPTS) -c -o $@ $<

# Headers, and the platform .c files included by src/usbtemp.c.
$(OBJECTS): $(wildcard include/*)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $(SFLAGS) $@

clean:
	rm -fr $(OBJECTS)
	rm -f $(TARGET)
