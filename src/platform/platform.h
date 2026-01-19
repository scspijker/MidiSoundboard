#ifndef PLATFORM_H
#define PLATFORM_H

// Platform detection and header includes
#if defined(__APPLE__)
    #include "midi.h"
    #include "audio.h"
#elif defined(ESP_PLATFORM)
    #include "midi.h"
    #include "audio.h"
#else
    #error "Unsupported platform"
#endif

#endif // PLATFORM_H
