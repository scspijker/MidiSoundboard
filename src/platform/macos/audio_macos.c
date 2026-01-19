#ifdef __APPLE__

#include "../audio.h"
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_ACTIVE_SOUNDS 10
#define BUFFER_SIZE_SAMPLES 4096  // ~93ms at 44.1kHz

static AudioQueueRef audio_queue = NULL;
static AudioQueueBufferRef buffers[3];
static uint32_t sample_rate = 44100;
static bool initialized = false;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Active sounds being mixed
static active_sound_t active_sounds[MAX_ACTIVE_SOUNDS] = {0};

static void mix_audio(int16_t *output, size_t sample_count) {
    memset(output, 0, sample_count * sizeof(int16_t));
    
    pthread_mutex_lock(&queue_mutex);
    
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        active_sound_t *sound = &active_sounds[i];
        if (!sound->is_active) continue;
        
        for (size_t j = 0; j < sample_count; j++) {
            if (sound->position >= sound->length) {
                if (sound->is_looping) {
                    sound->position = 0; // Loop back
                } else {
                    sound->is_active = false; // Sound finished
                    break;
                }
            }
            
            if (sound->is_active && sound->position < sound->length) {
                // Mix samples (simple addition with clipping)
                int32_t mixed = (int32_t)output[j] + (int32_t)sound->data[sound->position];
                output[j] = (int16_t)(mixed > 32767 ? 32767 : (mixed < -32768 ? -32768 : mixed));
                sound->position++;
            }
        }
    }
    
    pthread_mutex_unlock(&queue_mutex);
}

static void audio_callback(void *user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    (void)user_data;
    (void)queue;
    
    // Mix all active sounds
    mix_audio((int16_t *)buffer->mAudioData, buffer->mAudioDataByteSize / sizeof(int16_t));
    
    // Enqueue the buffer back
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

int audio_init(uint32_t sr) {
    if (initialized) {
        return 0;
    }
    
    sample_rate = sr;
    
    AudioStreamBasicDescription format = {0};
    format.mSampleRate = sample_rate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBytesPerPacket = 2;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = 2;
    format.mChannelsPerFrame = 1;
    format.mBitsPerChannel = 16;
    
    OSStatus status = AudioQueueNewOutput(&format, audio_callback, NULL, NULL, NULL, 0, &audio_queue);
    if (status != noErr) {
        fprintf(stderr, "Failed to create audio queue\n");
        return -1;
    }
    
    // Allocate buffers
    UInt32 buffer_size = BUFFER_SIZE_SAMPLES * sizeof(int16_t);
    for (int i = 0; i < 3; i++) {
        status = AudioQueueAllocateBuffer(audio_queue, buffer_size, &buffers[i]);
        if (status != noErr) {
            fprintf(stderr, "Failed to allocate audio buffer %d\n", i);
            audio_cleanup();
            return -1;
        }
        
        memset(buffers[i]->mAudioData, 0, buffer_size);
        buffers[i]->mAudioDataByteSize = buffer_size;
        AudioQueueEnqueueBuffer(audio_queue, buffers[i], 0, NULL);
    }
    
    status = AudioQueueStart(audio_queue, NULL);
    if (status != noErr) {
        fprintf(stderr, "Failed to start audio queue\n");
        audio_cleanup();
        return -1;
    }
    
    memset(active_sounds, 0, sizeof(active_sounds));
    initialized = true;
    return 0;
}

int audio_start_sound(const int16_t *samples, size_t sample_count, bool loop, bool hold) {
    if (!initialized || samples == NULL || sample_count == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    // Find an empty slot or reuse an existing one with the same data
    int slot = -1;
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (active_sounds[i].data == samples) {
            // Same sound - restart it
            slot = i;
            break;
        }
        if (slot == -1 && !active_sounds[i].is_active) {
            slot = i;
        }
    }
    
    if (slot == -1) {
        // All slots full - find oldest sound to replace
        slot = 0;
        for (int i = 1; i < MAX_ACTIVE_SOUNDS; i++) {
            if (active_sounds[i].position > active_sounds[slot].position) {
                slot = i;
            }
        }
    }
    
    active_sound_t *sound = &active_sounds[slot];
    sound->data = samples;
    sound->length = sample_count;
    sound->position = 0;
    sound->is_active = true;
    sound->is_looping = loop;
    sound->is_hold = hold;
    
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

int audio_stop_sound(const int16_t *samples) {
    if (!initialized || samples == NULL) {
        return -1;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    for (int i = 0; i < MAX_ACTIVE_SOUNDS; i++) {
        if (active_sounds[i].data == samples && active_sounds[i].is_active) {
            if (active_sounds[i].is_hold) {
                // For hold mode, stop immediately
                active_sounds[i].is_active = false;
            } else if (active_sounds[i].is_looping) {
                // For loop mode, stop immediately (toggle off)
                active_sounds[i].is_active = false;
                active_sounds[i].is_looping = false;
            } else {
                // For oneshot mode, stop immediately
                active_sounds[i].is_active = false;
            }
            pthread_mutex_unlock(&queue_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&queue_mutex);
    return -1; // Sound not found
}

int audio_play_sample(const int16_t *samples, size_t sample_count) {
    // Legacy function - just start a oneshot sound
    return audio_start_sound(samples, sample_count, false, false);
}

void audio_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    if (audio_queue != NULL) {
        AudioQueueStop(audio_queue, true);
        
        for (int i = 0; i < 3; i++) {
            if (buffers[i] != NULL) {
                AudioQueueFreeBuffer(audio_queue, buffers[i]);
            }
        }
        
        AudioQueueDispose(audio_queue, true);
        audio_queue = NULL;
    }
    
    memset(active_sounds, 0, sizeof(active_sounds));
    pthread_mutex_destroy(&queue_mutex);
    initialized = false;
}

#endif // __APPLE__
