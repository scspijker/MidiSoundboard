# MIDI Soundboard

A cross-platform ANSI C application that reads MIDI keyboard input and plays short soundbites through audio output. Supports Mac OS and ESP32 platforms.

## Features

- **MIDI Input**: Reads MIDI note on/off messages from connected MIDI keyboards (connects to ALL available MIDI sources)
- **Audio Output**: Plays soundbites through platform-specific audio systems
- **MP3 Support**: Loads MP3 audio files directly (no conversion needed)
- **JSON Configuration**: Simple JSON-based configuration for organizing sounds
- **Multi-Page Support**: Organize sounds into 11 pages (0-10) for different sound banks
- **Playback Modes**: Three playback modes - oneshot, loop, and hold
- **Volume Control**: Per-sound volume offset for balancing audio levels
- **Cross-Platform**: Single codebase works on both Mac OS and ESP32

## Project Structure

```
.
├── src/
│   ├── main.c                    # Main application (Mac OS)
│   ├── midi_soundboard.h         # Public API header
│   ├── midi_soundboard.c         # Core soundboard logic
│   └── platform/
│       ├── platform.h            # Platform abstraction
│       ├── midi.h                # MIDI interface
│       ├── audio.h               # Audio interface
│       ├── macos/
│       │   ├── midi_macos.c      # Mac OS MIDI implementation (CoreMIDI)
│       │   └── audio_macos.c     # Mac OS audio implementation (CoreAudio)
│       └── esp32/
│           ├── main_esp32.c      # ESP32 entry point
│           ├── midi_esp32.c      # ESP32 MIDI implementation (UART)
│           └── audio_esp32.c     # ESP32 audio implementation (I2S DAC)
├── Makefile                      # Build file for Mac OS
└── platformio.ini                # PlatformIO config for ESP32
```

## Building for Mac OS

### Prerequisites
- Xcode Command Line Tools
- GCC or Clang

### Build and Run

```bash
make
./midi_soundboard
```

The application will automatically connect to **all available MIDI sources** and start listening for MIDI events. It will load sounds from `sounds/config.json` (or a custom path if specified).

## Building for ESP32

### Prerequisites
- PlatformIO (install via VS Code extension or pip: `pip install platformio`)

### Hardware Setup

**MIDI Input:**
- Connect MIDI device to ESP32 UART2 (GPIO 16/17 by default)
- Use a MIDI-to-UART converter circuit (opto-isolator recommended)
- MIDI baud rate: 31250

**Audio Output:**
- Built-in DAC on GPIO 25 (left channel)
- Or connect external I2S DAC to:
  - BCK: GPIO 26
  - WS: GPIO 25
  - DATA: GPIO 22

### Build and Upload

```bash
pio run -t upload
pio device monitor
```

## Configuration

The application uses a JSON configuration file to define soundbites. Place your configuration file at `sounds/config.json` next to the executable, or specify a custom path as a command-line argument.

### Configuration File Format

The configuration file is a JSON object containing an array of sound entries:

```json
{
  "sounds": [
    {
      "filename": "kick.mp3",
      "page": 0,
      "note": 36,
      "volume_offset": 0.0,
      "color": [255, 0, 0],
      "mode": "oneshot"
    }
  ]
}
```

### Sound Entry Fields

Each sound entry in the `sounds` array must contain the following fields:

#### `filename` (string, required)
- The name of the MP3 file relative to the `sounds/` directory
- Example: `"kick.mp3"`, `"samples/snare.mp3"`

#### `page` (integer, required)
- Page number from **0 to 10**
- Allows organizing sounds into different pages/banks
- Only sounds on the current page will respond to MIDI input
- Example: `0` (first page), `5` (sixth page)

#### `note` (integer, required)
- MIDI note number from **0 to 127**
- The note that triggers this sound when pressed
- Standard MIDI note mapping:
  - `36` = C2 (kick drum)
  - `38` = D2 (snare drum)
  - `42` = F#2 (hi-hat)
  - `60` = C4 (middle C)
- Example: `36`, `60`, `127`

#### `volume_offset` (float, optional)
- Volume adjustment from **-1.0 to 1.0**
- Applied to the audio file without requiring normalization
- `0.0` = no change (original volume)
- `0.5` = 50% louder (1.5x volume)
- `-0.5` = 50% quieter (0.5x volume)
- `-1.0` = silent (0x volume)
- Useful for balancing different audio files
- Default: `0.0`
- Example: `0.0`, `-0.2`, `0.3`

#### `color` (array of 3 integers, optional)
- RGB color values from **0 to 255**
- Format: `[red, green, blue]`
- Used for visual feedback (e.g., button colors in a UI)
- Default: `[128, 128, 128]` (gray)
- Example: `[255, 0, 0]` (red), `[0, 255, 0]` (green), `[128, 128, 255]` (light blue)

#### `mode` (string, required)
- Playback mode, one of:
  - `"oneshot"` - Plays once when the note is pressed, stops when released
  - `"loop"` - Continuously loops while the note is held
  - `"hold"` - Plays once when pressed, continues until note is released again
- Example: `"oneshot"`, `"loop"`, `"hold"`

### Example Configuration

Here's a complete example configuration file:

```json
{
  "sounds": [
    {
      "filename": "kick.mp3",
      "page": 0,
      "note": 36,
      "volume_offset": 0.0,
      "color": [255, 0, 0],
      "mode": "oneshot"
    },
    {
      "filename": "snare.mp3",
      "page": 0,
      "note": 38,
      "volume_offset": 0.0,
      "color": [0, 255, 0],
      "mode": "oneshot"
    },
    {
      "filename": "hihat.mp3",
      "page": 0,
      "note": 42,
      "volume_offset": -0.2,
      "color": [0, 0, 255],
      "mode": "oneshot"
    },
    {
      "filename": "ambient_loop.mp3",
      "page": 1,
      "note": 60,
      "volume_offset": 0.1,
      "color": [128, 128, 255],
      "mode": "loop"
    },
    {
      "filename": "sustained_pad.mp3",
      "page": 2,
      "note": 48,
      "volume_offset": 0.0,
      "color": [255, 128, 0],
      "mode": "hold"
    }
  ]
}
```

### Directory Structure

Organize your project like this:

```
midi_soundboard/
├── midi_soundboard          # Executable
└── sounds/
    ├── config.json          # Configuration file
    ├── kick.mp3            # Audio files
    ├── snare.mp3
    ├── hihat.mp3
    └── ...
```

## Usage

1. **Prepare Your Sounds**: Place your MP3 files in the `sounds/` directory next to the executable

2. **Create Configuration**: Create a `config.json` file in the `sounds/` directory (see example above)

3. **Run the Application**:
   ```bash
   ./midi_soundboard
   ```
   
   Or specify a custom config path:
   ```bash
   ./midi_soundboard /path/to/config.json
   ```

4. **Play Sounds**: Press keys on your MIDI keyboard corresponding to the configured notes

5. **Change Pages**: Use MIDI CC messages to switch between pages (0-10)

6. **Exit**: On Mac OS, press Ctrl+C to exit. On ESP32, the application runs continuously.

## Platform-Specific Notes

### Mac OS
- Uses CoreMIDI for MIDI input
- Uses CoreAudio/AudioToolbox for audio output
- Automatically connects to the first available MIDI source
- Supports standard Mac audio devices

### ESP32
- Uses UART2 for MIDI input (configurable in `midi_esp32.c`)
- Uses I2S DAC for audio output
- Built-in DAC on GPIO 25, or external I2S DAC
- FreeRTOS-based multitasking
- Default sample rate: 44100 Hz

## Customization

### Adding More Sounds

Simply add entries to your `config.json` file:

```json
{
  "sounds": [
    {
      "filename": "new_sound.mp3",
      "page": 0,
      "note": 61,
      "volume_offset": 0.0,
      "color": [255, 255, 0],
      "mode": "oneshot"
    }
  ]
}
```

Place the corresponding MP3 file in the `sounds/` directory and restart the application.

### Changing MIDI Port (ESP32)

Edit `src/platform/esp32/midi_esp32.c` to change the UART number or pins:

```c
#define UART_NUM UART_NUM_2  // Change to UART_NUM_0, UART_NUM_1, etc.
```

### Changing Audio Pins (ESP32)

Edit `src/platform/esp32/audio_esp32.c` to change I2S pins:

```c
#define I2S_BCK_PIN GPIO_NUM_26
#define I2S_WS_PIN GPIO_NUM_25
#define I2S_DATA_PIN GPIO_NUM_22
```

## Troubleshooting

### Mac OS: "No MIDI sources found"
- Ensure your MIDI keyboard is connected and powered on
- Check System Preferences > Audio MIDI Setup to verify the device is recognized
- The application connects to ALL available MIDI sources automatically

### Configuration File Issues
- Ensure `config.json` is valid JSON (use a JSON validator if unsure)
- Check that MP3 file paths in the config are correct relative to the `sounds/` directory
- Verify all required fields (`filename`, `page`, `note`, `mode`) are present
- Ensure `page` is between 0-10 and `note` is between 0-127

### ESP32: No audio output
- Verify DAC/I2S pins are correctly connected
- Check that the sample rate matches your audio data
- Ensure audio_init() was called successfully

### ESP32: No MIDI input
- Verify MIDI device is connected to the correct UART pins
- Check MIDI baud rate is set to 31250
- Use a proper MIDI-to-UART converter circuit

## License

This project is provided as-is for educational and personal use. The included sounds are Creative Commons Zero from freesound.org.
