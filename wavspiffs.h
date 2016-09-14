#ifndef __WAVFILE_H__

#include <FS.h>

typedef struct wavFILE_s {
    File f;
} wavFILE_t;

typedef struct wavProperties_s {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;

#ifdef __cplusplus
extern "C" {
#endif

int wavOpen(const char *wavname, wavFILE_t *, wavProperties_t *wavProps);
int wavRead(wavFILE_t *wf, void *buffer, size_t buflen);
int wavClose(wavFILE_t *wf);

#ifdef __cplusplus
}
#endif

#endif
#define __WAVFILE_H__   1
