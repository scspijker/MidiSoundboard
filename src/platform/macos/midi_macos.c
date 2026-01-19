#ifdef __APPLE__

#include "../midi.h"
#include <CoreMIDI/CoreMIDI.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MIDI_SOURCES 32

static MIDIClientRef midi_client = 0;
static MIDIPortRef input_port = 0;
static MIDIEndpointRef sources[MAX_MIDI_SOURCES] = {0};
static ItemCount source_count = 0;
static midi_event_t pending_event = {0};
static bool has_pending_event = false;

static void midi_read_proc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon) {
    (void)refCon;
    (void)connRefCon;
    
    printf("[MIDI] Received packet list with %u packets\n", (unsigned int)pktlist->numPackets);
    
    const MIDIPacket *packet = &pktlist->packet[0];
    
    for (UInt32 i = 0; i < pktlist->numPackets; i++) {
        printf("[MIDI] Packet %u: length=%u, data=", (unsigned int)i, (unsigned int)packet->length);
        for (UInt16 j = 0; j < packet->length && j < 16; j++) {
            printf("%02X ", packet->data[j]);
        }
        printf("\n");
        
        if (packet->length >= 3) {
            uint8_t status = packet->data[0];
            uint8_t note = packet->data[1];
            uint8_t velocity = packet->data[2];
            
            // Check if it's a note on or note off message
            uint8_t message_type = status & 0xF0;
            
            printf("[MIDI] Status=0x%02X, Note=%u, Velocity=%u, Type=0x%02X\n", 
                   status, note, velocity, message_type);
            
            if (message_type == 0x90) { // Note On
                pending_event.note = note;
                pending_event.velocity = velocity;
                pending_event.is_on = (velocity > 0); // Note on with velocity 0 is actually note off
                has_pending_event = true;
                printf("[MIDI] Queued Note ON event: note=%u, velocity=%u\n", note, velocity);
            } else if (message_type == 0x80) { // Note Off
                pending_event.note = note;
                pending_event.velocity = velocity;
                pending_event.is_on = false;
                has_pending_event = true;
                printf("[MIDI] Queued Note OFF event: note=%u\n", note);
            } else {
                printf("[MIDI] Ignored message type 0x%02X\n", message_type);
            }
        } else {
            printf("[MIDI] Packet too short (length=%u), ignoring\n", (unsigned int)packet->length);
        }
        
        packet = MIDIPacketNext(packet);
    }
}

int midi_init(void) {
    OSStatus status;
    
    status = MIDIClientCreate(CFSTR("MIDI Soundboard"), NULL, NULL, &midi_client);
    if (status != noErr) {
        fprintf(stderr, "Failed to create MIDI client\n");
        return -1;
    }
    
    status = MIDIInputPortCreate(midi_client, CFSTR("Input Port"), midi_read_proc, NULL, &input_port);
    if (status != noErr) {
        fprintf(stderr, "Failed to create MIDI input port\n");
        MIDIClientDispose(midi_client);
        return -1;
    }
    
    // Find all available MIDI sources
    ItemCount total_sources = MIDIGetNumberOfSources();
    printf("[MIDI] Found %u MIDI source(s)\n", (unsigned int)total_sources);
    
    if (total_sources == 0) {
        fprintf(stderr, "[MIDI] ERROR: No MIDI sources found\n");
        MIDIPortDispose(input_port);
        MIDIClientDispose(midi_client);
        return -1;
    }
    
    // Limit to MAX_MIDI_SOURCES
    ItemCount sources_to_connect = total_sources;
    if (sources_to_connect > MAX_MIDI_SOURCES) {
        printf("[MIDI] WARNING: Limiting to %d sources (found %u)\n", MAX_MIDI_SOURCES, (unsigned int)total_sources);
        sources_to_connect = MAX_MIDI_SOURCES;
    }
    
    // Connect to all available sources
    source_count = 0;
    for (ItemCount i = 0; i < sources_to_connect; i++) {
        MIDIEndpointRef src = MIDIGetSource(i);
        CFStringRef name = NULL;
        MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);
        
        char nameBuf[256] = "(unnamed)";
        if (name) {
            CFStringGetCString(name, nameBuf, sizeof(nameBuf), kCFStringEncodingUTF8);
        }
        
        printf("[MIDI] Connecting to source %u: %s\n", (unsigned int)i, nameBuf);
        
        status = MIDIPortConnectSource(input_port, src, NULL);
        if (status != noErr) {
            fprintf(stderr, "[MIDI] WARNING: Failed to connect to source %u: %s (status=%d)\n", 
                    (unsigned int)i, nameBuf, (int)status);
            if (name) CFRelease(name);
            continue; // Try next source
        }
        
        sources[source_count] = src;
        source_count++;
        printf("[MIDI] Successfully connected to source %u: %s\n", (unsigned int)i, nameBuf);
        
        if (name) CFRelease(name);
    }
    
    if (source_count == 0) {
        fprintf(stderr, "[MIDI] ERROR: Failed to connect to any MIDI sources\n");
        MIDIPortDispose(input_port);
        MIDIClientDispose(midi_client);
        return -1;
    }
    
    printf("[MIDI] Successfully connected to %u MIDI source(s)\n", (unsigned int)source_count);
    return 0;
}

int midi_read(midi_event_t *event) {
    if (event == NULL) {
        return -1;
    }
    
    if (has_pending_event) {
        *event = pending_event;
        has_pending_event = false;
        return 0;
    }
    
    return 1; // No event available
}

void midi_cleanup(void) {
    // Disconnect all sources
    if (input_port != 0) {
        for (ItemCount i = 0; i < source_count; i++) {
            if (sources[i] != 0) {
                MIDIPortDisconnectSource(input_port, sources[i]);
                printf("[MIDI] Disconnected from source %u\n", (unsigned int)i);
            }
        }
    }
    
    if (input_port != 0) {
        MIDIPortDispose(input_port);
        input_port = 0;
    }
    
    if (midi_client != 0) {
        MIDIClientDispose(midi_client);
        midi_client = 0;
    }
    
    // Clear sources array
    for (ItemCount i = 0; i < source_count; i++) {
        sources[i] = 0;
    }
    source_count = 0;
    has_pending_event = false;
}

#endif // __APPLE__
