// CMedia.h - a sound resource: raw PCM data decoded from WAV / Ogg Vorbis / IMA ADPCM.
#ifndef CMEDIA_H
#define CMEDIA_H

#include <wchar.h>
#include "CBinary.h"

namespace com { namespace glu { namespace platform { namespace components {

class CStrWChar;

/** "fmt " chunk of a WAV file plus the size of its "data" chunk. */
struct WavFormat {
    unsigned short m_formatTag;         // 0x00 1 = PCM
    unsigned short m_channels;          // 0x02
    unsigned int m_sampleRate;          // 0x04
    unsigned int m_byteRate;            // 0x08
    unsigned short m_blockAlign;        // 0x0c
    unsigned short m_bitsPerSample;     // 0x0e
    unsigned int m_dataSize;            // 0x10
};

class CMedia : public CBinary {
public:
    enum { ClassId = 0x027b32f1 };

    /** Mime keys handled by Load(). */
    enum {
        MimeWav = 0xfd8a7754,
        MimeOgg = 0x32fd99e1,
        MimeAdpcm = 0x32fd9f47,
        MimePcm = 0xfd94b3c1
    };

    /** Message ids sent when a media becomes available / goes away. */
    enum {
        MessageLoaded = 0xd8d9fc09,
        MessageDestroyed = 0x369989ef
    };

    CMedia()
    {
        m_classId = ClassId;
        m_name = NULL;
        m_channels = 0;
        m_sampleRate = 0;
        m_bitsPerSample = 0;
        m_isLoaded = 0;
    }

    virtual ~CMedia();

    virtual void Destroy();
    /** Decode `stream` according to its mime key `type` into PCM; `name` is kept for debugging. */
    virtual unsigned char Load(CInputStream& stream, unsigned int type, const wchar_t* name);

    /** Parse the RIFF/WAVE header up to the start of the "data" chunk. */
    unsigned char ReadWavHeader(CInputStream& stream, WavFormat& format);
    unsigned char GetDescription(unsigned int& channels, unsigned int& bitsPerSample, unsigned int& sampleRate,
                                 const CStrWChar** name);

    CStrWChar* m_name;                  // 0x14
    unsigned int m_channels;            // 0x18
    unsigned int m_sampleRate;          // 0x1c
    unsigned int m_bitsPerSample;       // 0x20
    unsigned char m_isLoaded;           // 0x24 MessageLoaded has been sent
};

} } } }

#endif
