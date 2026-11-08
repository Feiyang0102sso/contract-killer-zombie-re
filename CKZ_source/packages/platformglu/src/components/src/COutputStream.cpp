// COutputStream.cpp - base output stream: typed writes with optional byte swap, bounded sub-streams.
#include "COutputStream.h"

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

COutputStream::COutputStream()
{
    m_target = NULL;
    m_isBigEndian = 0;
    m_error = 0;
    m_length = 0;
    m_position = 0;
}

COutputStream::~COutputStream()
{
}

void COutputStream::Close()
{
    m_target = NULL;
    m_isBigEndian = 0;
    m_error = 0;
    m_length = 0;
    m_position = 0;
}

void COutputStream::WriteUInt8(unsigned char value)
{
    Write(&value, 1);
}

void COutputStream::Write(const unsigned char* data, unsigned int size)
{
    if (m_target != NULL) {
        if (size != 0) {
            if (size <= m_length - m_position + 1) {
                m_target->Write(data, size);
                m_error = m_target->m_error;
                m_position += size;
            } else {
                m_error = 1;
            }
        }
    } else {
        WriteInternal(data, size);
    }
}

void COutputStream::WriteUInt16(unsigned short value)
{
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    Write((unsigned char*)&value, 2);
}

void COutputStream::WriteUInt32(unsigned int value)
{
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    Write((unsigned char*)&value, 4);
}

void COutputStream::WriteInt16(short value)
{
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    Write((unsigned char*)&value, 2);
}

void COutputStream::WriteInt32(int value)
{
    if (m_isBigEndian) {
        SwapBytes((unsigned char*)&value, sizeof(value));
    }
    Write((unsigned char*)&value, 4);
}

/** No backend: never open. */
unsigned char COutputStream::IsOpenInternal()
{
    return 0;
}

/** No backend: any real write fails. */
void COutputStream::WriteInternal(const unsigned char* data, unsigned int size)
{
    if (data != NULL && size != 0) {
        m_error = 1;
    }
}

} } } }
