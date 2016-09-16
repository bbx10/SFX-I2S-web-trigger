/*****************************************************************************
  The MIT License (MIT)

  Copyright (c) 2016 by bbx10node@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 **************************************************************************/

/*
 * Read WAV/RIFF audio files. Parses header and returns properties. This is
 * mostly designed for uncompressed PCM audio. The WAV files must be stored
 * in the ESP8266 SPIFFS file system.
 */

#include "wavspiffs.h"

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

static int readuint32(wavFILE_t *wf, uint32_t *ui32)
{
    int rc;

    rc = wf->f.read((uint8_t *)ui32, sizeof(*ui32));
    //Serial.printf("readuint32 rc=%d val=0x%X\r\n", rc, *ui32);
    return rc;
}

int wavOpen(const char *wavname, wavFILE_t *wf, wavProperties_t *wavProps)
{
    typedef enum headerState_e {
        HEADER_INIT, HEADER_RIFF, HEADER_FMT, HEADER_DATA
    } headerState_t;
    headerState_t state = HEADER_INIT;
    uint32_t chunkID, chunkSize;

    wf->f = SPIFFS.open(wavname, "r");
    if (!wf->f) return -1;
    Serial.println("SPIFFS.open ok");

    while (state != HEADER_DATA) {
        if (readuint32(wf, &chunkID) != 4) return -1;
        if (readuint32(wf, &chunkSize) != 4) return -2;
        switch (chunkID) {
            case CCCC('R', 'I', 'F', 'F'):
                if (readuint32(wf, &chunkID) != 4) return -3;
                if (chunkID != CCCC('W', 'A', 'V', 'E')) return -4;
                state = HEADER_RIFF;
                Serial.printf("RIFF %d\r\n", chunkSize);
                break;

            case CCCC('f', 'm', 't', ' '):
                if (wf->f.read((uint8_t *)wavProps, sizeof(*wavProps)) !=
                        sizeof(*wavProps)) return -5;
                state = HEADER_FMT;
                Serial.printf("fmt  %d\r\n", chunkSize);
                if (chunkSize > sizeof(*wavProps)) {
                    wf->f.seek(chunkSize - sizeof(*wavProps), SeekCur);
                }
                break;

            case CCCC('d', 'a', 't', 'a'):
                state = HEADER_DATA;
                Serial.printf("data %d\r\n", chunkSize);
                break;
            default:
                Serial.printf("%08X %d\r\n", chunkID, chunkSize);
                if (!wf->f.seek(chunkSize, SeekCur)) return -6;
        }
    }
    if (state == HEADER_DATA) return 0;
    wf->f.close();
    return -7;
}

int wavRead(wavFILE_t *wf, void *buffer, size_t buflen)
{
    return wf->f.read((uint8_t *)buffer, buflen);
}

int wavClose(wavFILE_t *wf)
{
    wf->f.close();
    return 0;
}
