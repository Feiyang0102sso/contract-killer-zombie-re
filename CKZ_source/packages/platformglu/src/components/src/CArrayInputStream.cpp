// CArrayInputStream.cpp - input stream over a memory block.
#include "CArrayInputStream.h"
#include "CUtf.h"

namespace com { namespace glu { namespace platform { namespace components {

CArrayInputStream::CArrayInputStream()
{
    m_data = NULL;
}

CArrayInputStream::~CArrayInputStream()
{
    Close();
}

void CArrayInputStream::Close()
{
    CInputStream::Close();
    m_data = NULL;
}

unsigned char CArrayInputStream::Open(const unsigned char* data, unsigned int size)
{
    Close();
    m_error = 1;
    if (data != NULL && size != 0) {
        m_data = data;
        m_length = size;
        m_error = 0;
        return 1;
    }
    return 0;
}

unsigned int CArrayInputStream::ReadUtf8(unsigned int byteCount, wchar_t* out, unsigned int outCapacity)
{
    if (out == NULL || outCapacity == 0) {
        return 0;
    }
    out[0] = 0;
    unsigned int count = 0;
    if (Available() >= byteCount) {
        count = core::CUtf::Utf8ToWcs(m_data + m_position, byteCount, out, outCapacity);
        m_position += byteCount;
    } else {
        m_error = 1;
    }
    return count;
}

unsigned int CArrayInputStream::ReadJMUtf(wchar_t* out, unsigned int outCapacity)
{
    // the length prefix is always big-endian
    unsigned char savedBigEndian = m_isBigEndian;
    m_isBigEndian = 1;
    unsigned short byteCount = ReadUInt16();
    m_isBigEndian = savedBigEndian;
    return ReadUtf8(byteCount, out, outCapacity);
}

unsigned char CArrayInputStream::IsOpenInternal()
{
    return m_data != NULL && m_length != 0;
}

unsigned int CArrayInputStream::AvailableInternal()
{
    if (m_data == NULL) {
        return 0;
    }
    if (m_error) {
        return 0;
    }
    return m_length - m_position;
}

void CArrayInputStream::SkipInternal(unsigned int size)
{
    if (size == 0) {
        return;
    }
    if (Available() >= size) {
        m_position += size;
    } else {
        m_error = 1;
    }
}

void CArrayInputStream::ReadInternal(unsigned char* buffer, unsigned int size)
{
    if (buffer == NULL || size == 0) {
        return;
    }
    if (Available() < size) {
        m_error = 1;
    } else {
        memcpy(buffer, m_data + m_position, size);
        m_position += size;
        if (!m_error) {
            return;
        }
    }
    memset(buffer, 0, size);
}

void CArrayInputStream::ResetInternal()
{
}

} } } }
