// CArrayOutputStream.h - output stream writing into a caller-owned memory block.
#ifndef CARRAYOUTPUTSTREAM_H
#define CARRAYOUTPUTSTREAM_H

#include <wchar.h>
#include "COutputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

class CArrayOutputStream : public COutputStream {
public:
    CArrayOutputStream();
    virtual ~CArrayOutputStream();

    /** Write into `data` (not owned) of `size` bytes. */
    unsigned char Open(unsigned char* data, unsigned int size);
    virtual void Close();
    /** Java-style string: big-endian uint16 byte length followed by UTF-8 bytes. */
    void WriteJMUtf(const wchar_t* text);

    virtual unsigned char IsOpenInternal();
    virtual void WriteInternal(const unsigned char* data, unsigned int size);

    unsigned char* m_data;          // 0x14
};

} } } }

#endif
