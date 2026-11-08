// CZipInputStream.h - input stream inflating zlib data read from a source stream.
// Compressed bytes are pulled from the source into a 512-byte input buffer and inflated in
// 1 KB chunks into the output buffer that ReadInternal/SkipInternal consume.
// (first drafted by b005 as PROVISIONAL; completed by b006, the owner)
#ifndef CZIPINPUTSTREAM_H
#define CZIPINPUTSTREAM_H

#include "CInputStream.h"
#include "zlib.h"

namespace com { namespace glu { namespace platform { namespace components {

class CZipInputStream : public CInputStream {
public:
    /** Compressed bytes buffered from the source at a time. */
    static const unsigned int InputBufferSize = 0x200;
    /** Bytes inflated per chunk. */
    static const unsigned int OutputBufferSize = 0x400;

    CZipInputStream();
    virtual ~CZipInputStream();

    virtual void Close();
    /** Inflate `compressedSize` bytes of `source` into `uncompressedSize` bytes. */
    unsigned char Open(CInputStream& source, unsigned int compressedSize, unsigned int uncompressedSize);

    virtual unsigned char IsOpenInternal();
    virtual unsigned int AvailableInternal();
    virtual void SkipInternal(unsigned int size);
    /** Refill the output buffer: top up the input buffer, then inflate one chunk. */
    void ZipReadChunk();
    virtual void ReadInternal(unsigned char* buffer, unsigned int size);
    /** Rewind the source and restart inflating from the beginning. */
    virtual void ResetInternal();

    unsigned char m_inflateActive;                  // 0x1c inflateInit succeeded, not ended yet
    z_stream m_stream;                              // 0x20
    unsigned char m_input[InputBufferSize];         // 0x58 compressed bytes
    unsigned int m_inputSize;                       // 0x258 bytes in m_input
    unsigned int m_inputPos;                        // 0x25c bytes of m_input already inflated
    unsigned char m_output[OutputBufferSize];       // 0x260 inflated bytes
    unsigned int m_outputSize;                      // 0x660 bytes in m_output
    unsigned int m_outputPos;                       // 0x664 bytes of m_output already returned
    CInputStream* m_compressed;                     // 0x668 source stream (not owned)
    unsigned int m_compressedSize;                  // 0x66c
    unsigned int m_compressedRead;                  // 0x670 compressed bytes taken from the source
    unsigned int m_uncompressedSize;                // 0x674
    unsigned int m_uncompressedRead;                // 0x678 inflated bytes handed out
};

} } } }

#endif
