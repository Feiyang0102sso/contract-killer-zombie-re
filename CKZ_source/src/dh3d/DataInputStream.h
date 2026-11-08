// DataInputStream.h - big-endian reader over a byte buffer (java.io.DataInputStream style).
#ifndef DATAINPUTSTREAM_H
#define DATAINPUTSTREAM_H

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace components {
class CBinary;
} } } }

/** Reads big-endian values from a buffer it does not own. */
class JDataInputStream {
public:
    JDataInputStream(char* data, int size);
    JDataInputStream(com::glu::platform::components::CBinary* binary);
    ~JDataInputStream();

    char readByte();
    short readShort();
    int readInt();
    int readUnsignedShort();
    /** Zero terminated string; the caller frees it with delete[]. */
    char* readUTF();
    /** Length prefixed (one byte) string of 16-bit characters; the caller frees it with delete[]. */
    wchar_t* readWChar();
    /** value * 2^(exponent - 1), used to decode the float exponent (only correct for value 2). */
    float pow(float value, int exponent);
    /** IEEE single decoded bit by bit. */
    float readFloat();

    char* m_data;           // 0x00
    int m_position;         // 0x04 read cursor
    int m_size;             // 0x08
    bool m_ownsData;        // 0x0c cleared by the (char*, int) constructor only
};

#endif
