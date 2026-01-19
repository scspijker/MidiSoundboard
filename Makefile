# Makefile for Mac OS build

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -framework CoreMIDI -framework CoreAudio -framework AudioToolbox -framework CoreFoundation -lm -lpthread

SRCDIR = src
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/midi_soundboard.c \
          $(SRCDIR)/config.c \
          $(SRCDIR)/audio_loader_macos.c \
          $(SRCDIR)/platform/macos/midi_macos.c \
          $(SRCDIR)/platform/macos/audio_macos.c

OBJECTS = $(SOURCES:.c=.o)
TARGET = midi_soundboard

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
