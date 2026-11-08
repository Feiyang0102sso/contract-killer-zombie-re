// COutputStream.h - base class of every Glu output stream (mirror of CInputStream).
// A stream either writes to its own backend (the *Internal virtuals) or, when m_target is set,
// forwards into that stream while counting bytes against m_length.
// (first drafted by b005 as PROVISIONAL; completed by b006, the owner)
#ifndef COUTPUTSTREAM_H
#define COUTPUTSTREAM_H

namespace com { namespace glu { namespace platform { namespace components {

class COutputStream {
public:
    COutputStream();
    virtual ~COutputStream();

    /** Detach from the target and reset all state. */
    virtual void Close();

    void WriteUInt8(unsigned char value);
    /** Write `size` bytes; sets m_error when they do not fit. */
    void Write(const unsigned char* data, unsigned int size);
    void WriteUInt16(unsigned short value);
    void WriteUInt32(unsigned int value);
    void WriteInt16(short value);
    void WriteInt32(int value);

    // backend hooks, used when m_target is NULL
    virtual unsigned char IsOpenInternal();
    virtual void WriteInternal(const unsigned char* data, unsigned int size);

    COutputStream* m_target;        // 0x04 stream this one writes through (NULL = own backend)
    unsigned char m_isBigEndian;    // 0x08 write multi-byte values big-endian
    unsigned char m_error;          // 0x09 a write did not fit
    unsigned int m_length;          // 0x0c capacity
    unsigned int m_position;        // 0x10 bytes written so far
};

} } } }

#endif
