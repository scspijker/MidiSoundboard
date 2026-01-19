#ifndef PLATFORM_MIDI_H
#define PLATFORM_MIDI_H

#include "../midi_soundboard.h"

// Platform-specific MIDI implementation
int midi_init(void);
int midi_read(midi_event_t *event);
void midi_cleanup(void);

#endif // PLATFORM_MIDI_H
