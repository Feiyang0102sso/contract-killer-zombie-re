// CInputStream.h - base class of every Glu input stream.
// A stream either reads from its own backend (the *Internal virtuals) or, when opened on a
// source stream, acts as a bounded window [0, length) over that source.
#ifndef CINPUTSTREAM_H
#define CINPUTSTREAM_H

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream {
public:
    /** Called from Close()/destructor of a stream opened with a close handler. */
    typedef unsigned char (*CloseHandler)(void* context, CInputStream& stream);

    CInputStream();
    virtual ~CInputStream();

    /** Open as a window of `length` bytes over `source`. */
    unsigned char Open(CInputStream* source, unsigned int length);
    /** Same as above, `onClose(context, *this)` is called when the stream closes. */
    unsigned char Open(CInputStream* source, unsigned int length, void* closeContext, CloseHandler onClose);
    virtual void Close();

    unsigned char ReadUInt8();
    void Read(unsigned char* buffer, unsigned int size);
    unsigned short ReadUInt16();
    unsigned int ReadUInt32();
    signed char ReadInt8();
    short ReadInt16();
    int ReadInt32();
    unsigned int Available();
    void Skip(unsigned int size);

    void Reset();
    static void Reset(CInputStream* stream);
    void Mark();
    static void Mark(CInputStream* stream);

    // backend hooks, used when m_source is NULL
    virtual unsigned char IsOpenInternal();
    virtual unsigned int AvailableInternal();
    virtual void SkipInternal(unsigned int size);
    virtual void ReadInternal(unsigned char* buffer, unsigned int size);
    virtual void ResetInternal();
    virtual void MarkInternal();

    CInputStream* m_source;         // 0x04 stream this one is a window over (NULL = own backend)
    unsigned char m_isBigEndian;    // 0x08 multi-byte values are stored big-endian (swap on read)
    unsigned char m_error;          // 0x09 a read/skip went past the end
    unsigned int m_length;          // 0x0c window length / data size
    unsigned int m_position;        // 0x10 bytes consumed so far
    void* m_closeContext;           // 0x14 first argument of m_onClose
    CloseHandler m_onClose;         // 0x18 close notification
};

} } } }

#endif
