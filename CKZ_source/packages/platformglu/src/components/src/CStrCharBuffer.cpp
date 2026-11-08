// CStrCharBuffer.cpp - growable char string builder.
#include "CStrCharBuffer.h"

namespace com { namespace glu { namespace platform { namespace components {

void CStrCharBuffer::EnsureCapacity(int capacity)
{
    if (m_capacity >= capacity) {
        return;
    }

    m_capacity = capacity;
    char* buffer = new char[capacity + 1];
    buffer[0] = '\0';
    memcpy(buffer, m_string, m_length);
    if (m_string != NULL) {
        delete[] m_string;
        m_string = NULL;
    }
    buffer[m_length] = '\0';
    m_string = buffer;
}

CStrCharBuffer& CStrCharBuffer::Append(const char* text)
{
    int textLength = strlen(text);
    EnsureCapacity(m_length + textLength);
    memcpy(m_string + m_length, text, textLength);
    m_length += textLength;
    m_string[m_length] = '\0';
    return *this;
}

} } } }
