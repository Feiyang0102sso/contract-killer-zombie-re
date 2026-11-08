// CADPCMInputStream.h - input stream that decodes IMA ADPCM from a source stream into 16-bit PCM.
// The source is read in blocks of up to 64 bytes; each block decodes to 4x as many PCM bytes.
#ifndef CADPCMINPUTSTREAM_H
#define CADPCMINPUTSTREAM_H

#include "CInputStream.h"
#include "adpcm.h"

namespace com { namespace glu { namespace platform { namespace adpcm {

class CADPCMInputStream : public components::CInputStream {
public:
    /** Magic of a Glu ADPCM file ("IMA " read as a 32-bit value). */
    enum { FileMagic = 0x494d4120 };
    /** Compressed bytes decoded per block. */
    enum { InputBlockSize = 64 };

    CADPCMInputStream();
    virtual ~CADPCMInputStream();

    /**
     * Decode `length` bytes of `source`. A file with an "IMA " header supplies the format,
     * otherwise the media player's output format is assumed and `source` is rewound.
     */
    unsigned char Open(components::CInputStream& source, unsigned int length, unsigned int& channels,
                       unsigned int& bitsPerSample, unsigned int& sampleRate);
    virtual void Close();
    virtual unsigned char IsOpenInternal();
    virtual unsigned int AvailableInternal();
    virtual void SkipInternal(unsigned int size);
    /** Decode the next block of the source into m_output. */
    void ADPCMReadChunk();
    virtual void ReadInternal(unsigned char* buffer, unsigned int size);
    virtual void ResetInternal();

    unsigned char m_isOpen;                     // 0x01c more compressed data to decode
    adpcm_state m_decoderState;                 // 0x01e
    signed char m_input[InputBlockSize];        // 0x024 compressed block
    unsigned char m_output[InputBlockSize * 4];  // 0x064 decoded block (16-bit PCM samples)
    unsigned int m_decodedLength;               // 0x164 valid bytes in m_output
    unsigned int m_decodedPosition;             // 0x168 bytes of m_output already returned
    components::CInputStream* m_compressed;     // 0x16c ADPCM source
    unsigned int m_compressedLength;            // 0x170 ADPCM bytes to decode
    unsigned int m_compressedRead;              // 0x174 ADPCM bytes read so far
    unsigned int m_pcmLength;                   // 0x178 total PCM bytes
    unsigned int m_pcmPosition;                 // 0x17c PCM bytes returned so far
    unsigned int m_channels;                    // 0x180
};

} } } }

#endif
