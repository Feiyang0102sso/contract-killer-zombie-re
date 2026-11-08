// CArrayOutputStream.cpp - output stream into a memory block.
#include "CArrayOutputStream.h"
#include "CUtf.h"

namespace com { namespace glu { namespace platform { namespace components {

CArrayOutputStream::CArrayOutputStream()
{
    m_data = NULL;
}

CArrayOutputStream::~CArrayOutputStream()
{
}

unsigned char CArrayOutputStream::Open(unsigned char* data, unsigned int size)
{
    m_error = 1;
    if (data != NULL && size != 0) {
        m_data = data;
        m_length = size;
        m_error = 0;
        return 1;
    }
    return 0;
}

void CArrayOutputStream::Close()
{
    COutputStream::Close();
    m_data = NULL;
}

void CArrayOutputStream::WriteJMUtf(const wchar_t* text)
{
    if (text == NULL) {
        return;
    }
    unsigned int utfLength = core::CUtf::GetUtfLen(text, wcslen(text));
    unsigned int bufferSize = utfLength + 1;
    unsigned char* buffer = (unsigned char*)malloc(bufferSize);
    if (buffer != NULL) {
        // the length prefix is always big-endian
        unsigned char savedBigEndian = m_isBigEndian;
        m_isBigEndian = 1;
        WriteUInt16((unsigned short)utfLength);
        m_isBigEndian = savedBigEndian;
        unsigned int written = core::CUtf::WcsToUtf8(text, wcslen(text), buffer, bufferSize);
        Write(buffer, written);
        free(buffer);
    } else {
        m_error = 1;
    }
}

unsigned char CArrayOutputStream::IsOpenInternal()
{
    return m_data != NULL && m_length != 0;
}

/** Copy as many bytes as fit; m_error tells whether everything fitted. */
void CArrayOutputStream::WriteInternal(const unsigned char* data, unsigned int size)
{
    m_error = 1;
    if (m_data == NULL || data == NULL) {
        return;
    }
    m_error = (m_length + 1 - m_position < size);
    for (unsigned int i = 0; i < size && m_position < m_length; i++) {
        m_data[m_position++] = data[i];
    }
}

} } } }
