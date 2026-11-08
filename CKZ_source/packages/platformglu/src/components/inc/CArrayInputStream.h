// CArrayInputStream.h - input stream reading from a caller-owned memory block.
#ifndef CARRAYINPUTSTREAM_H
#define CARRAYINPUTSTREAM_H

#include <wchar.h>
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

class CArrayInputStream : public CInputStream {
public:
    CArrayInputStream();
    virtual ~CArrayInputStream();

    virtual void Close();
    /** Read from `data` (not copied, must outlive the stream). */
    unsigned char Open(const unsigned char* data, unsigned int size);
    /** Decode `byteCount` UTF-8 bytes into `out`; returns characters written. */
    unsigned int ReadUtf8(unsigned int byteCount, wchar_t* out, unsigned int outCapacity);
    /** Java-style string: big-endian uint16 byte length followed by UTF-8 bytes. */
    unsigned int ReadJMUtf(wchar_t* out, unsigned int outCapacity);

    virtual unsigned char IsOpenInternal();
    virtual unsigned int AvailableInternal();
    virtual void SkipInternal(unsigned int size);
    virtual void ReadInternal(unsigned char* buffer, unsigned int size);
    virtual void ResetInternal();

    const unsigned char* m_data;    // 0x1c
};

} } } }

#endif
