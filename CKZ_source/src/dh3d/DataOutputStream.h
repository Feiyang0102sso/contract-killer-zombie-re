// DataOutputStream.h - big-endian writer into a growable byte buffer (java.io.DataOutputStream style).
#ifndef DATAOUTPUTSTREAM_H
#define DATAOUTPUTSTREAM_H

#include <stddef.h>
#include <wchar.h>
#include "Vector_ss.h"

class DataOutputStream {
public:
    /** How writeWStr stores a wide string (every character as 2 big-endian bytes). */
    enum wchar_format_t {
        WCHAR_TERMINATED = 0,       // characters followed by the terminator character
        WCHAR_RAW = 1,              // characters only
        WCHAR_LENGTH_PREFIXED = 2   // one length byte, then the characters
    };

    DataOutputStream();

    void writeByte(unsigned char value);
    void writeInt(int value);
    void writeWStr(const wchar_t* text, wchar_format_t format, wchar_t terminator);
    unsigned char* GetData() const;
    /** Drop everything written so far and release the buffer. */
    void clear();
    void write(unsigned char* bytes, unsigned int count);

    Vector<unsigned char> m_buffer;     // 0x00
};

#endif
