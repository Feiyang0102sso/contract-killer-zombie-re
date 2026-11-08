// CArrayStream_gServe.h - gserve's memory streams: the platform array streams plus
// "JMUtf32" strings (UTF-8 bytes behind a 32-bit big-endian byte count instead of 16-bit).
// (b020; the classes are constructed in persistence/CProfileManager.cpp)
#ifndef CARRAYSTREAM_GSERVE_H
#define CARRAYSTREAM_GSERVE_H

#include <wchar.h>
#include "CArrayInputStream.h"
#include "CArrayOutputStream.h"

class CArrayInputStream_gServe : public com::glu::platform::components::CArrayInputStream {
public:
    CArrayInputStream_gServe() {}
    virtual ~CArrayInputStream_gServe() {}

    /** Big-endian uint32 byte length followed by UTF-8 bytes; returns characters written. */
    unsigned int ReadJMUtf32(wchar_t* out, unsigned int outCapacity);
};

class CArrayOutputStream_gServe : public com::glu::platform::components::CArrayOutputStream {
public:
    CArrayOutputStream_gServe() {}
    virtual ~CArrayOutputStream_gServe() {}

    /** Write `text` as a big-endian uint32 byte length followed by UTF-8 bytes (NULL is ignored). */
    void WriteJMUtf32(const wchar_t* text);
};

#endif
