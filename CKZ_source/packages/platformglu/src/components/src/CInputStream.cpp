// CInputStream.cpp - base input stream: typed reads with optional byte swap, bounded sub-streams.
#include "CInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Reverse the byte order of a value in place. */
static inline void SwapBytes(unsigned char* bytes, unsigned int size)
{
    for (unsigned int i = 0; i < size / 2; i++) {
        unsigned char low = bytes[i];
        bytes[i] = bytes[size - 1 - i];
        bytes[size - 1 - i] = low;
    }
}

CInputStream::CInputStream()
{
    m_source = NULL;
    m_isBigEndian = 0;
    m_error = 0;
    m_length = 0;
    m_position = 0;
    m_closeContext = NULL;
    m_onClose = NULL;
}

CInputStream::~CInputStream()
{
    if (m_onClose != NULL) {
        m_onClose(m_closeContext, *this);
    }
}

unsigned char CInputStream::Open(CInputStream* source, unsigned int length)
{
    Close();
    if (source != NULL && length != 0) {
        m_source = source;
        m_length = length;
        return 1;
    }
    return 0;
}

unsigned char CInputStream::Open(CInputStream* source, unsigned int length, void* closeContext, CloseHandler onClose)
{
    Close();
    if (source != NULL && length != 0) {
        m_source = source;
        m_length = length;
        m_closeContext = closeContext;
        m_onClose = onClose;
        return 1;
    }
    return 0;
}

void CInputStream::Close()
{
    if (m_onClose != NULL) {
        m_onClose(m_closeContext, *this);
    }
    m_closeContext = NULL;
    m_onClose = NULL;
    m_source = NULL;
    m_isBigEndian = 0;
    m_error = 0;
    m_length = 0;
    m_position = 0;
}

unsigned char CInputStream::ReadUInt8()
{
    unsigned char value = 0;
    Read(&value, 1);
    return value;
}

/** Read `size` bytes; on failure the buffer is zero-filled and m_error is set. */
void CInputStream::Read(unsigned char* buffer, unsigned int size)
{
    if (m_source != NULL) {
        if (size == 0) {
            return;
        }
        if (Available() < size) {
            m_error = 1;
        } else {
            m_source->Read(buffer, size);
            m_error = m_source->m_error;
            m_position += size;
            if (!m_error) {
                return;
            }
        }
        memset(buffer, 0, size);
        return;
    }
    ReadInternal(buffer, size);
}

unsigned short CInputStream::ReadUInt16()
{
    unsigned short value = 0;
    Read((unsigned char*)&value, 2);
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    return value;
}

unsigned int CInputStream::ReadUInt32()
{
    unsigned int value = 0;
    Read((unsigned char*)&value, 4);
    unsigned char* bytes = (unsigned char*)&value;
    if (m_isBigEndian) {
        SwapBytes(bytes, sizeof(value));
    }
    return value;
}

signed char CInputStream::ReadInt8()
{
    signed char value = 0;
    Read((unsigned char*)&value, 1);
    return value;
}

short CInputStream::ReadInt16()
{
    short value = 0;
    Read((unsigned char*)&value, 2);
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    return value;
}

int CInputStream::ReadInt32()
{
    int value = 0;
    Read((unsigned char*)&value, 4);
    unsigned char* bytes = (unsigned char*)&value;
    if (m_isBigEndian) {
        SwapBytes(bytes, sizeof(value));
    }
    return value;
}

/** Bytes left: the window remainder, limited by what the source still has. */
unsigned int CInputStream::Available()
{
    if (m_source != NULL) {
        if (m_error) {
            return 0;
        }
        unsigned int remaining = m_length - m_position;
        unsigned int sourceAvailable = m_source->Available();
        if (remaining < sourceAvailable) {
            return remaining;
        }
        return sourceAvailable;
    }
    return AvailableInternal();
}

void CInputStream::Skip(unsigned int size)
{
    if (m_source != NULL) {
        if (size == 0) {
            return;
        }
        if (Available() < size) {
            m_error = 1;
            return;
        }
    } else {
        SkipInternal(size);
        return;
    }
    m_source->Skip(size);
    m_error = m_source->m_error;
    m_position += size;
}

unsigned char CInputStream::IsOpenInternal()
{
    return 0;
}

unsigned int CInputStream::AvailableInternal()
{
    return 0;
}

void CInputStream::SkipInternal(unsigned int size)
{
    if (size != 0) {
        m_error = 1;
    }
}

void CInputStream::ReadInternal(unsigned char* buffer, unsigned int size)
{
    if (buffer != NULL && size != 0) {
        memset(buffer, 0, size);
        m_error = 1;
    }
}

/** Rewind this stream and every stream below it. */
void CInputStream::Reset()
{
    if (m_source != NULL) {
        m_source->Reset();
    }
    ResetInternal();
}

void CInputStream::Reset(CInputStream* stream)
{
    stream->Reset();
}

void CInputStream::ResetInternal()
{
    m_error = 0;
    m_position = 0;
}

/** Mark the current position on the bottom-most stream. */
void CInputStream::Mark()
{
    CInputStream* stream = this;
    while (stream->m_source != NULL) {
        stream = stream->m_source;
    }
    stream->MarkInternal();
}

void CInputStream::Mark(CInputStream* stream)
{
    while (stream->m_source != NULL) {
        stream = stream->m_source;
    }
    stream->MarkInternal();
}

void CInputStream::MarkInternal()
{
}

} } } }
