#ifdef __APPLE__

#include "audio_loader.h"
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int audio_load_file(const char *filepath, audio_data_t *audio) {
    if (!filepath || !audio) {
        return -1;
    }
    
    memset(audio, 0, sizeof(*audio));
    
    CFStringRef path = CFStringCreateWithCString(NULL, filepath, kCFStringEncodingUTF8);
    if (!path) {
        fprintf(stderr, "[AUDIO] Failed to create CFString for path\n");
        return -1;
    }
    
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
    CFRelease(path);
    
    if (!url) {
        fprintf(stderr, "[AUDIO] Failed to create URL\n");
        return -1;
    }
    
    // Use ExtAudioFile for conversion (handles MP3 and other formats)
    ExtAudioFileRef extAudioFile;
    OSStatus status = ExtAudioFileOpenURL(url, &extAudioFile);
    CFRelease(url);
    
    if (status != noErr) {
        fprintf(stderr, "[AUDIO] Failed to open audio file: %s (status=%d)\n", filepath, (int)status);
        return -1;
    }
    
    // Get source format
    AudioStreamBasicDescription sourceFormat;
    UInt32 size = sizeof(sourceFormat);
    status = ExtAudioFileGetProperty(extAudioFile, kExtAudioFileProperty_FileDataFormat, &size, &sourceFormat);
    
    if (status != noErr) {
        ExtAudioFileDispose(extAudioFile);
        fprintf(stderr, "[AUDIO] Failed to get source format\n");
        return -1;
    }
    
    // Set output format (16-bit PCM mono)
    AudioStreamBasicDescription outputFormat = {0};
    outputFormat.mSampleRate = sourceFormat.mSampleRate;
    outputFormat.mFormatID = kAudioFormatLinearPCM;
    outputFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    outputFormat.mBitsPerChannel = 16;
    outputFormat.mChannelsPerFrame = 1; // Force mono
    outputFormat.mBytesPerFrame = 2;
    outputFormat.mFramesPerPacket = 1;
    outputFormat.mBytesPerPacket = 2;
    
    status = ExtAudioFileSetProperty(extAudioFile, kExtAudioFileProperty_ClientDataFormat,
                                    sizeof(outputFormat), &outputFormat);
    
    if (status != noErr) {
        ExtAudioFileDispose(extAudioFile);
        fprintf(stderr, "[AUDIO] Failed to set output format\n");
        return -1;
    }
    
    // Get file length
    SInt64 numFrames = 0;
    size = sizeof(numFrames);
    ExtAudioFileGetProperty(extAudioFile, kExtAudioFileProperty_FileLengthFrames, &size, &numFrames);
    
    size_t bufferSize = numFrames * outputFormat.mBytesPerFrame;
    int16_t *buffer = malloc(bufferSize);
    
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mBuffers[0].mDataByteSize = bufferSize;
    bufferList.mBuffers[0].mData = buffer;
    
    UInt32 numFramesRead = (UInt32)numFrames;
    status = ExtAudioFileRead(extAudioFile, &numFramesRead, &bufferList);
    
    ExtAudioFileDispose(extAudioFile);
    
    if (status != noErr) {
        free(buffer);
        fprintf(stderr, "[AUDIO] Failed to read audio data\n");
        return -1;
    }
    
    audio->data = buffer;
    audio->sample_count = numFramesRead;
    audio->sample_rate = (uint32_t)outputFormat.mSampleRate;
    audio->channels = 1;
    
    printf("[AUDIO] Loaded %s: %zu samples @ %u Hz\n", filepath, audio->sample_count, audio->sample_rate);
    return 0;
}

void audio_free(audio_data_t *audio) {
    if (audio && audio->data) {
        free(audio->data);
        audio->data = NULL;
        audio->sample_count = 0;
        audio->sample_rate = 0;
        audio->channels = 0;
    }
}

#endif // __APPLE__
