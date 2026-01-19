#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple JSON parser for our specific format
static void skip_whitespace(const char **json) {
    while (**json && isspace(**json)) {
        (*json)++;
    }
}

static int parse_string(const char **json, char **out) {
    skip_whitespace(json);
    if (**json != '"') return -1;
    (*json)++;
    
    const char *start = *json;
    while (**json && **json != '"') {
        if (**json == '\\') (*json)++;
        (*json)++;
    }
    
    if (**json != '"') return -1;
    size_t len = *json - start;
    (*json)++;
    
    *out = malloc(len + 1);
    memcpy(*out, start, len);
    (*out)[len] = '\0';
    return 0;
}

static int parse_number(const char **json, int *out) {
    skip_whitespace(json);
    char *end;
    *out = (int)strtol(*json, &end, 10);
    if (end == *json) return -1;
    *json = end;
    return 0;
}

static int parse_float(const char **json, float *out) {
    skip_whitespace(json);
    char *end;
    *out = strtof(*json, &end);
    if (end == *json) return -1;
    *json = end;
    return 0;
}

static int parse_sound_entry(const char **json, sound_config_t *sound) {
    skip_whitespace(json);
    if (**json != '{') return -1;
    (*json)++;
    
    memset(sound, 0, sizeof(*sound));
    sound->mode = SOUND_MODE_ONESHOT; // Default
    
    while (**json && **json != '}') {
        skip_whitespace(json);
        
        char *key = NULL;
        if (parse_string(json, &key) != 0) {
            (*json)++;
            continue;
        }
        
        skip_whitespace(json);
        if (**json != ':') {
            free(key);
            (*json)++;
            continue;
        }
        (*json)++;
        skip_whitespace(json);
        
        if (strcmp(key, "filename") == 0) {
            parse_string(json, &sound->filename);
        } else if (strcmp(key, "page") == 0) {
            int val;
            if (parse_number(json, &val) == 0) {
                if (val >= 0 && val <= 10) {
                    sound->page = (uint8_t)val;
                } else {
                    fprintf(stderr, "[CONFIG] Invalid page: %d (must be 0-10)\n", val);
                }
            }
        } else if (strcmp(key, "note") == 0) {
            int val;
            if (parse_number(json, &val) == 0) {
                if (val >= 0 && val <= 127) {
                    sound->note = (uint8_t)val;
                } else {
                    fprintf(stderr, "[CONFIG] Invalid note: %d (must be 0-127)\n", val);
                }
            }
        } else if (strcmp(key, "volume_offset") == 0) {
            float vol;
            if (parse_float(json, &vol) == 0) {
                if (vol >= -1.0f && vol <= 1.0f) {
                    sound->volume_offset = vol;
                } else {
                    fprintf(stderr, "[CONFIG] Invalid volume_offset: %f (must be -1.0 to 1.0)\n", vol);
                    sound->volume_offset = 0.0f;
                }
            }
        } else if (strcmp(key, "color") == 0) {
            skip_whitespace(json);
            if (**json == '[') {
                (*json)++;
                int r, g, b;
                parse_number(json, &r);
                skip_whitespace(json);
                if (**json == ',') (*json)++;
                parse_number(json, &g);
                skip_whitespace(json);
                if (**json == ',') (*json)++;
                parse_number(json, &b);
                skip_whitespace(json);
                if (**json == ']') (*json)++;
                sound->color_r = (uint8_t)r;
                sound->color_g = (uint8_t)g;
                sound->color_b = (uint8_t)b;
            }
        } else if (strcmp(key, "mode") == 0) {
            char *mode_str = NULL;
            if (parse_string(json, &mode_str) == 0) {
                if (strcmp(mode_str, "loop") == 0) {
                    sound->mode = SOUND_MODE_LOOP;
                } else if (strcmp(mode_str, "oneshot") == 0) {
                    sound->mode = SOUND_MODE_ONESHOT;
                } else if (strcmp(mode_str, "hold") == 0) {
                    sound->mode = SOUND_MODE_HOLD;
                }
                free(mode_str);
            }
        }
        
        free(key);
        skip_whitespace(json);
        if (**json == ',') (*json)++;
    }
    
    if (**json == '}') (*json)++;
    return 0;
}

int config_load(const char *json_path, config_t *config) {
    FILE *f = fopen(json_path, "r");
    if (!f) {
        fprintf(stderr, "[CONFIG] Failed to open config file: %s\n", json_path);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size <= 0 || size > 10 * 1024 * 1024) { // Max 10MB
        fprintf(stderr, "[CONFIG] Invalid file size: %ld\n", size);
        fclose(f);
        return -1;
    }
    
    char *json = malloc(size + 1);
    if (!json) {
        fprintf(stderr, "[CONFIG] Failed to allocate memory\n");
        fclose(f);
        return -1;
    }
    
    size_t bytes_read = fread(json, 1, size, f);
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "[CONFIG] Failed to read entire file\n");
        free(json);
        fclose(f);
        return -1;
    }
    json[size] = '\0';
    fclose(f);
    
    memset(config, 0, sizeof(*config));
    
    // Extract base path
    const char *last_slash = strrchr(json_path, '/');
    if (last_slash) {
        size_t path_len = last_slash - json_path + 1;
        config->base_path = malloc(path_len + 1);
        memcpy(config->base_path, json_path, path_len);
        config->base_path[path_len] = '\0';
    } else {
        config->base_path = strdup("./");
    }
    
    // Count sounds
    const char *p = json;
    config->sound_count = 0;
    while (*p) {
        if (strstr(p, "\"filename\"") != NULL) {
            config->sound_count++;
            p = strstr(p, "\"filename\"") + 10;
        } else {
            break;
        }
    }
    
    if (config->sound_count == 0) {
        fprintf(stderr, "[CONFIG] No sounds found in config\n");
        free(json);
        return -1;
    }
    
    config->sounds = calloc(config->sound_count, sizeof(sound_config_t));
    
    // Parse sounds array
    p = json;
    skip_whitespace(&p);
    if (*p == '{') {
        // Find "sounds" key
        p = strstr(p, "\"sounds\"");
        if (p) {
            p = strchr(p, '[');
            if (p) p++;
        }
    } else if (*p == '[') {
        p++;
    }
    
    for (size_t i = 0; i < config->sound_count; i++) {
        parse_sound_entry(&p, &config->sounds[i]);
        skip_whitespace(&p);
        if (*p == ',') p++;
    }
    
    free(json);
    printf("[CONFIG] Loaded %zu sound(s) from %s\n", config->sound_count, json_path);
    return 0;
}

void config_free(config_t *config) {
    if (!config) return;
    
    for (size_t i = 0; i < config->sound_count; i++) {
        free(config->sounds[i].filename);
    }
    free(config->sounds);
    free(config->base_path);
    memset(config, 0, sizeof(*config));
}

sound_config_t *config_find_sound(const config_t *config, uint8_t page, uint8_t note) {
    if (!config) return NULL;
    
    for (size_t i = 0; i < config->sound_count; i++) {
        if (config->sounds[i].page == page && config->sounds[i].note == note) {
            return &config->sounds[i];
        }
    }
    return NULL;
}
