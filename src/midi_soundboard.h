#ifndef MIDI_SOUNDBOARD_H
#define MIDI_SOUNDBOARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"  // For sound_mode_t enum

// MIDI note structure
typedef struct {
    uint8_t note;      // MIDI note number (0-127)
    uint8_t velocity;  // Note velocity (0-127)
    bool is_on;        // true for note on, false for note off
} midi_event_t;

// Platform abstraction functions
// MIDI input
int midi_init(void);
int midi_read(midi_event_t *event);
void midi_cleanup(void);

// Audio output
int audio_init(uint32_t sample_rate);
int audio_play_sample(const int16_t *samples, size_t sample_count);
void audio_cleanup(void);

// Soundbite management
typedef struct {
    int16_t *data;              // Audio data (owned by soundboard)
    size_t length;
    uint32_t sample_rate;
    float volume_offset;         // Volume adjustment (-1.0 to 1.0)
    uint8_t page;                // Page number (0-10)
    uint8_t color_r, color_g, color_b; // RGB color
    sound_mode_t mode;           // Playback mode (use enum from config.h)
    bool is_playing;             // Currently playing (for loop/hold mode)
} soundbite_t;

int soundboard_init(void);
int soundboard_load_soundbite(uint8_t page, uint8_t note, const int16_t *data, size_t length, uint32_t sample_rate, float volume_offset, sound_mode_t mode);
int soundboard_play_note(uint8_t page, uint8_t note);
int soundboard_stop_note(uint8_t page, uint8_t note);
uint8_t soundboard_get_current_page(void);
void soundboard_set_page(uint8_t page);
void soundboard_cleanup(void);

#endif // MIDI_SOUNDBOARD_H
