#ifndef AUDIO_LOADER_H
#define AUDIO_LOADER_H

#include <stdint.h>
#include <stddef.h>

// Loaded audio data
typedef struct {
    int16_t *data;              // Audio samples (16-bit PCM)
    size_t sample_count;         // Number of samples
    uint32_t sample_rate;        // Sample rate (Hz)
    size_t channels;             // Number of channels (1=mono, 2=stereo)
} audio_data_t;

// Load audio file (MP3, WAV, etc.) and convert to PCM
int audio_load_file(const char *filepath, audio_data_t *audio);
void audio_free(audio_data_t *audio);

#endif // AUDIO_LOADER_H
