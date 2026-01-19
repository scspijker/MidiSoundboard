#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Playback modes
typedef enum {
    SOUND_MODE_LOOP = 0,
    SOUND_MODE_ONESHOT = 1,
    SOUND_MODE_HOLD = 2
} sound_mode_t;

// Sound configuration entry
typedef struct {
    char *filename;           // MP3 filename
    uint8_t page;              // Page number (0-10)
    uint8_t note;              // MIDI note (0-127)
    float volume_offset;        // Volume adjustment (-1.0 to 1.0)
    uint8_t color_r;            // RGB color red component (0-255)
    uint8_t color_g;            // RGB color green component (0-255)
    uint8_t color_b;            // RGB color blue component (0-255)
    sound_mode_t mode;          // Playback mode
} sound_config_t;

// Configuration structure
typedef struct {
    sound_config_t *sounds;     // Array of sound configurations
    size_t sound_count;         // Number of sounds
    char *base_path;            // Base path to sounds folder
} config_t;

// Configuration functions
int config_load(const char *json_path, config_t *config);
void config_free(config_t *config);
sound_config_t *config_find_sound(const config_t *config, uint8_t page, uint8_t note);

#endif // CONFIG_H
