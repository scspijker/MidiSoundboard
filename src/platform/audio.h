#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Active sound track for mixing
typedef struct {
    const int16_t *data;        // Audio data
    size_t length;              // Total length in samples
    size_t position;            // Current playback position
    bool is_active;             // Is this track currently playing
    bool is_looping;            // Should this track loop
    bool is_hold;               // Hold mode - stops when note off
} active_sound_t;

// Platform-specific audio implementation
int audio_init(uint32_t sample_rate);
int audio_start_sound(const int16_t *samples, size_t sample_count, bool loop, bool hold);
int audio_stop_sound(const int16_t *samples);  // Stop by data pointer
void audio_cleanup(void);

// Legacy function for backward compatibility (now just starts a sound)
int audio_play_sample(const int16_t *samples, size_t sample_count);

#endif // PLATFORM_AUDIO_H
