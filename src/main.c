#include "midi_soundboard.h"
#include "config.h"
#include "audio_loader.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include <mach-o/dyld.h>
#elif defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static volatile bool running = true;

#ifdef __APPLE__
void signal_handler(int sig) {
    (void)sig;
    running = false;
}
#endif

static int load_sounds_from_config(const char *config_path) {
    config_t config;
    if (config_load(config_path, &config) != 0) {
        return -1;
    }
    
    char filepath[1024];
    int loaded_count = 0;
    
    for (size_t i = 0; i < config.sound_count; i++) {
        sound_config_t *sound_cfg = &config.sounds[i];
        
        // Build full path
        snprintf(filepath, sizeof(filepath), "%s%s", config.base_path, sound_cfg->filename);
        
        printf("[MAIN] Loading sound: %s (page=%u, note=%u)\n", 
               filepath, sound_cfg->page, sound_cfg->note);
        
        audio_data_t audio;
        if (audio_load_file(filepath, &audio) != 0) {
            fprintf(stderr, "[MAIN] Failed to load: %s\n", filepath);
            continue;
        }
        
        if (soundboard_load_soundbite(sound_cfg->page, sound_cfg->note,
                                     audio.data, audio.sample_count, audio.sample_rate,
                                     sound_cfg->volume_offset, sound_cfg->mode) != 0) {
            fprintf(stderr, "[MAIN] Failed to register soundbite\n");
            audio_free(&audio);
            continue;
        }
        
        // Note: audio.data is now owned by soundboard, don't free it here
        loaded_count++;
    }
    
    config_free(&config);
    printf("[MAIN] Successfully loaded %d sound(s)\n", loaded_count);
    return loaded_count > 0 ? 0 : -1;
}

#ifdef ESP_PLATFORM
void app_main(void) {
#else
int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Determine config path
    char config_path[1024] = "sounds/config.json";
    if (argc > 1) {
        strncpy(config_path, argv[1], sizeof(config_path) - 1);
        config_path[sizeof(config_path) - 1] = '\0';
    } else {
        // Try multiple locations
        FILE *test = fopen(config_path, "r");
        if (!test) {
            // Try src/sounds/config.json (for development)
            strncpy(config_path, "src/sounds/config.json", sizeof(config_path) - 1);
            test = fopen(config_path, "r");
        }
        if (!test) {
            // Try next to executable
#ifdef __APPLE__
            char exe_path[1024];
            uint32_t size = sizeof(exe_path);
            if (_NSGetExecutablePath(exe_path, &size) == 0) {
                char *dir = dirname(exe_path);
                snprintf(config_path, sizeof(config_path), "%s/sounds/config.json", dir);
                test = fopen(config_path, "r");
            }
#endif
        }
        if (test) {
            fclose(test);
        }
    }
#endif
    
    printf("MIDI Soundboard starting...\n");
    printf("Config path: %s\n", config_path);
    
    if (soundboard_init() != 0) {
        printf("Failed to initialize soundboard\n");
#ifdef ESP_PLATFORM
        return;
#else
        return 1;
#endif
    }
    
    if (load_sounds_from_config(config_path) != 0) {
        printf("Failed to load sounds from config\n");
        soundboard_cleanup();
#ifdef ESP_PLATFORM
        return;
#else
        return 1;
#endif
    }
    
    printf("MIDI Soundboard ready. Press keys on your MIDI keyboard.\n");
    printf("Current page: %u (use MIDI CC to change pages)\n", soundboard_get_current_page());
#ifdef __APPLE__
    printf("Press Ctrl+C to exit.\n");
#endif
    
    midi_event_t event;
    unsigned long loop_count = 0;
    while (running) {
        int result = midi_read(&event);
        if (result == 0) {
            uint8_t page = soundboard_get_current_page();
            
            if (event.is_on) {
                printf("[MAIN] Note ON: %d (velocity: %d) on page %u\n", 
                       event.note, event.velocity, page);
                soundboard_play_note(page, event.note);
            } else {
                printf("[MAIN] Note OFF: %d on page %u\n", event.note, page);
                soundboard_stop_note(page, event.note);
            }
        } else if (result < 0) {
            printf("[MAIN] ERROR: midi_read() returned error\n");
        }
        
        // Print status every 5 seconds
        loop_count++;
        if (loop_count % 5000 == 0) {
            printf("[MAIN] Still running... (page %u)\n", soundboard_get_current_page());
        }
        
#ifdef __APPLE__
        usleep(1000); // Small delay to prevent busy waiting
#else
        vTaskDelay(pdMS_TO_TICKS(1));
#endif
    }
    
    printf("\nShutting down...\n");
    soundboard_cleanup();
    
#ifndef ESP_PLATFORM
    return 0;
#endif
}
