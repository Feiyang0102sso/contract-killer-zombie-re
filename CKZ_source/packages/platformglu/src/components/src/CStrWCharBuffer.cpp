// CStrWCharBuffer.cpp - growable wide string builder.
#include "CStrWCharBuffer.h"
#include "ICStdUtil.h"

namespace com { namespace glu { namespace platform { namespace components {

void CStrWCharBuffer::EnsureCapacity(int capacity)
{
    if (m_capacity >= capacity) {
        return;
    }

    m_capacity = capacity;
    wchar_t* buffer = new wchar_t[capacity + 1];
    buffer[0] = 0;
    memcpy(buffer, m_string, m_length * sizeof(wchar_t));
    if (m_string != NULL) {
        delete[] m_string;
        m_string = NULL;
    }
    buffer[m_length] = 0;
    m_string = buffer;
}

CStrWCharBuffer& CStrWCharBuffer::Append(const wchar_t* text)
{
    int textLength = wcslen(text);
    EnsureCapacity(m_length + textLength);
    memcpy(m_string + m_length, text, textLength * sizeof(wchar_t));
    m_length += textLength;
    m_string[m_length] = 0;
    return *this;
}

CStrWCharBuffer& CStrWCharBuffer::Append(const wchar_t* text, int start, unsigned long count)
{
    // leftover of a debug length check: the result is unused, but wcslen is not known to be
    // side-effect free, so the call stays in the binary
    wcslen(text);
    EnsureCapacity(m_length + count);
    memcpy(m_string + m_length, text + start, count * sizeof(wchar_t));
    m_length += count;
    m_string[m_length] = 0;
    return *this;
}

CStrWCharBuffer& CStrWCharBuffer::Insert(int position, const char* text)
{
    int textLength = strlen(text);
    EnsureCapacity(m_length + textLength);
    memmove(m_string + position + textLength, m_string + position, (m_length - position) * sizeof(wchar_t));
    // note: mbstowcs also writes the terminator, over the first moved character
    mbstowcs(m_string + position, text, strlen(text) + 1);
    m_length += textLength;
    m_string[m_length] = 0;
    return *this;
}

CStrWCharBuffer& CStrWCharBuffer::Insert(int position, int value)
{
    char digits[16];
    core::ICStdUtil::SPrintF(digits, "%ld\0", value);
    return Insert(position, digits);
}

CStrWCharBuffer& CStrWCharBuffer::Replace(const wchar_t* pattern, const wchar_t* replacement, int searchStart,
                                          unsigned long searchLength, int patternStart, unsigned long patternLength)
{
    if (wcslen(pattern) == 0 || searchLength < patternLength) {
        return *this;
    }

    const wchar_t* wanted = pattern + patternStart;
    int replacementLength = wcslen(replacement);
    int growth = replacementLength - patternLength;
    int lastPosition = searchStart + searchLength - patternLength;
    int position = searchStart;
    while (position <= lastPosition) {
        unsigned long matched = 0;
        while (matched < patternLength && m_string[position + matched] == wanted[matched]) {
            matched++;
        }
        if (matched < patternLength) {
            position++;
            continue;
        }

        EnsureCapacity(m_length + growth);
        memmove(m_string + position + replacementLength, m_string + position + patternLength,
                (m_length - position - patternLength) * sizeof(wchar_t));
        memcpy(m_string + position, replacement, replacementLength * sizeof(wchar_t));
        m_length += growth;
        m_string[m_length] = 0;
        lastPosition += growth;
        position += replacementLength;
    }
    return *this;
}

} } } }
