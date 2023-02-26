/*
 * OpenAL example
 *
 * Copyright(C) Florian Fainelli <f.fainelli@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>

#include <AL/al.h>
#include <AL/alc.h>

#ifdef LIBAUDIO
#include <audio/wave.h>
#define BACKEND "libaudio"
#else
#include <AL/alut.h>
#define BACKEND "alut"
#endif

static void list_audio_devices(const ALCchar *devices)
{
    const ALCchar *device = devices, *next = devices + 1;
    size_t len = 0;

    fprintf(stdout, "Devices list:\n");
    fprintf(stdout, "----------\n");
    while (device && *device != '\0' && next && *next != '\0') {
        fprintf(stdout, "%s\n", device);
        len = strlen(device);
        device += (len + 1);
        next += (len + 2);
    }
    printf("----------\n");
}

#define TEST_ERROR(_msg)        \
    error = alGetError();       \
    if (error != AL_NO_ERROR) { \
        printf(_msg "\n"); \
        return -1;      \
    }

static inline ALenum to_al_format(short channels, short samples)
{
    bool stereo = (channels > 1);

    switch (samples) {
    case 16:
        if (stereo)
            return AL_FORMAT_STEREO16;
        else
            return AL_FORMAT_MONO16;
    case 8:
        if (stereo)
            return AL_FORMAT_STEREO8;
        else
            return AL_FORMAT_MONO8;
    default:
        printf("ERROR IN WAV");
        return -1;
    }
}
ALboolean enumeration;
//   const ALCchar *devices;
ALCchar *defaultDeviceName = NULL ;
int ret;
#ifdef LIBAUDIO
WaveInfo *wave;
#endif
char *bufferData;
ALCdevice *device;
ALvoid *data;
ALCcontext *context;
ALsizei size, freq;
ALenum format;
ALuint buffer, source;
ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
ALboolean loop = AL_FALSE;
ALCenum error;
ALint source_state;


int main1(void)
{

    printf("Using " BACKEND " as audio backend\n");

    enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
    if (enumeration == AL_FALSE)
        printf( "enumeration extension not available\n");

    list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

    if (!defaultDeviceName) {
        printf("Entered");
        defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
        printf("EXITED");
    }
    printf("Entered");
    device = alcOpenDevice(defaultDeviceName);
    if (!device) {
        printf("unable to open default device\n");
        return -1;
    }
    printf("Exited");

    printf( "Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

    alGetError();

    context = alcCreateContext(device, NULL);
    if (!alcMakeContextCurrent(context)) {
        printf("failed to make default context\n");
        return -1;
    }
    TEST_ERROR("make default context");

    /* set orientation */
    alListener3f(AL_POSITION, 0, 0, 0);
    TEST_ERROR("listener position");
        alListener3f(AL_VELOCITY, 0, 0, 0);
    TEST_ERROR("listener velocity");
    alListenerfv(AL_ORIENTATION, listenerOri);
    TEST_ERROR("listener orientation");

    alGenSources((ALuint)1, &source);
    TEST_ERROR("source generation");

    alSourcef(source, AL_PITCH, 1);
    TEST_ERROR("source pitch");
    alSourcef(source, AL_GAIN, 1);
    TEST_ERROR("source gain");
    alSource3f(source, AL_POSITION, 0, 0, 0);
    TEST_ERROR("source position");
    alSource3f(source, AL_VELOCITY, 0, 0, 0);
    TEST_ERROR("source velocity");
    alSourcei(source, AL_LOOPING, AL_FALSE);
    TEST_ERROR("source looping");

    alGenBuffers(1, &buffer);
    TEST_ERROR("buffer generation");

#ifdef LIBAUDIO
    /* load data */
    wave = WaveOpenFileForReading("test.wav");
    if (!wave) {
        printf("failed to read wave file\n");
        return -1;
    }

    ret = WaveSeekFile(0, wave);
    if (ret) {
        printf("failed to seek wave file\n");
        return -1;
    }

    bufferData = malloc(wave->dataSize);
    if (!bufferData) {
        printf("malloc");
        return -1;
    }

    ret = WaveReadFile(bufferData, wave->dataSize, wave);
    if (ret != wave->dataSize) {
        printf("short read: %d, want: %d\n", ret, wave->dataSize);
        return -1;
    }

    alBufferData(buffer, to_al_format(wave->channels, wave->bitsPerSample),
            bufferData, wave->dataSize, wave->sampleRate);
    TEST_ERROR("failed to load buffer data");
#else
    alutLoadWAVFile("test.wav", &format, &data, &size, &freq, &loop);
    TEST_ERROR("loading wav file");

    alBufferData(buffer, format, data, size, freq);
    TEST_ERROR("buffer copy");
#endif

    alSourcei(source, AL_BUFFER, buffer);
    TEST_ERROR("buffer binding");
return 0;
}

float f = 0;
int lr = 0;
int main2()
{
    if (lr == 0) {
        lr = 1;

        alSource3f(source, AL_POSITION, 0, 10, 0);
    } else {
        lr = 0;
        alSource3f(source, AL_POSITION, 10, 0, 0);
    }
    alSourcePlay(source);
    TEST_ERROR("source playing");

    alGetSourcei(source, AL_SOURCE_STATE, &source_state);
    TEST_ERROR("source state get");
//    float f = 10.0f;
    while (source_state == AL_PLAYING) {
        alGetSourcei(source, AL_SOURCE_STATE, &source_state);
  //  alSource3f(source, AL_POSITION, 0, f, f);
//f = (f + 1)*5;
        TEST_ERROR("source state get");
    }
    return 0;
}

int main3()
{
    /* exit context */
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
printf("FNAL");
}
