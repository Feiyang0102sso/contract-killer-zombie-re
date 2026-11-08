// CArrayStream_gServe.cpp - 32-bit length prefixed UTF-8 strings on the array streams.
#include "CArrayStream_gServe.h"
#include "CUtf.h"

using namespace com::glu::platform;

unsigned int CArrayInputStream_gServe::ReadJMUtf32(wchar_t* out, unsigned int outCapacity)
{
    // the length prefix is always big-endian
    unsigned char savedBigEndian = m_isBigEndian;
    m_isBigEndian = 1;
    unsigned int byteCount = ReadUInt32();
    m_isBigEndian = savedBigEndian;
    return ReadUtf8(byteCount, out, outCapacity);
}

void CArrayOutputStream_gServe::WriteJMUtf32(const wchar_t* text)
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
        WriteUInt32(utfLength);
        m_isBigEndian = savedBigEndian;
        unsigned int written = core::CUtf::WcsToUtf8(text, wcslen(text), buffer, bufferSize);
        Write(buffer, written);
        free(buffer);
    } else {
        m_error = 1;
    }
}
