#include "midi_soundboard.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_NOTES 128
#define MAX_PAGES 11

typedef struct {
    soundbite_t soundbites[MAX_NOTES];
} page_t;

static page_t pages[MAX_PAGES] = {0};
static uint8_t current_page = 0;
static bool initialized = false;

int soundboard_init(void) {
    if (initialized) {
        return 0;
    }
    
    memset(pages, 0, sizeof(pages));
    current_page = 0;
    
    if (midi_init() != 0) {
        return -1;
    }
    
    if (audio_init(44100) != 0) {
        midi_cleanup();
        return -1;
    }
    
    initialized = true;
    return 0;
}

int soundboard_load_soundbite(uint8_t page, uint8_t note, const int16_t *data, size_t length, 
                              uint32_t sample_rate, float volume_offset, uint8_t mode) {
    if (page >= MAX_PAGES || note >= MAX_NOTES || data == NULL || length == 0) {
        return -1;
    }
    
    soundbite_t *sb = &pages[page].soundbites[note];
    
    // Free existing data if any
    if (sb->data) {
        free(sb->data);
    }
    
    // Allocate and copy data
    sb->data = malloc(length * sizeof(int16_t));
    if (!sb->data) {
        return -1;
    }
    
    // Apply volume offset
    float volume_mult = 1.0f + volume_offset;
    volume_mult = fmaxf(0.0f, fminf(2.0f, volume_mult)); // Clamp to 0-2x
    
    for (size_t i = 0; i < length; i++) {
        float sample = (float)data[i] * volume_mult;
        sb->data[i] = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, sample));
    }
    
    sb->length = length;
    sb->sample_rate = sample_rate;
    sb->volume_offset = volume_offset;
    sb->page = page;
    sb->mode = mode;
    sb->is_playing = false;
    
    return 0;
}

int soundboard_play_note(uint8_t page, uint8_t note) {
    if (page >= MAX_PAGES || note >= MAX_NOTES) {
        return -1;
    }
    
    soundbite_t *sb = &pages[page].soundbites[note];
    if (sb->data == NULL || sb->length == 0) {
        return -1; // No soundbite loaded for this note
    }
    
    // Handle different modes
    bool loop = false;
    bool hold = false;
    
    if (sb->mode == 0) { // LOOP mode - toggle on/off
        if (sb->is_playing) {
            // Already playing - stop it (toggle off)
            sb->is_playing = false;
            return audio_stop_sound(sb->data);
        } else {
            // Not playing - start it (toggle on)
            loop = true;
            hold = false;
            sb->is_playing = true;
        }
    } else if (sb->mode == 2) { // HOLD mode
        loop = false;
        hold = true;
        if (sb->is_playing) {
            return 0; // Already playing
        }
        sb->is_playing = true;
    } else { // ONESHOT mode (1)
        loop = false;
        hold = false;
    }
    
    return audio_start_sound(sb->data, sb->length, loop, hold);
}

int soundboard_stop_note(uint8_t page, uint8_t note) {
    if (page >= MAX_PAGES || note >= MAX_NOTES) {
        return -1;
    }
    
    soundbite_t *sb = &pages[page].soundbites[note];
    if (sb->data == NULL || sb->length == 0) {
        return -1;
    }
    
    if (sb->mode == 2) { // HOLD mode - stop when note released
        sb->is_playing = false;
        return audio_stop_sound(sb->data);
    } else if (sb->mode == 0) { // LOOP mode - note off doesn't stop (toggle only)
        // Loop mode is toggled by note on, not note off
        return 0;
    }
    // ONESHOT mode - already finished, nothing to stop
    
    return 0;
}

uint8_t soundboard_get_current_page(void) {
    return current_page;
}

void soundboard_set_page(uint8_t page) {
    if (page < MAX_PAGES) {
        current_page = page;
    }
}

void soundboard_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    // Free all soundbite data
    for (int p = 0; p < MAX_PAGES; p++) {
        for (int n = 0; n < MAX_NOTES; n++) {
            if (pages[p].soundbites[n].data) {
                free(pages[p].soundbites[n].data);
                pages[p].soundbites[n].data = NULL;
            }
        }
    }
    
    midi_cleanup();
    audio_cleanup();
    memset(pages, 0, sizeof(pages));
    initialized = false;
}
