// CWStringBuffer.cpp - growable wide string builder used by the gServe JSON encoder.
#include "CWStringBuffer.h"

CWStringBuffer::~CWStringBuffer()
{
    if (m_data != NULL) {
        delete[] m_data;
        m_data = NULL;
    }
    m_length = 0;
    m_capacity = 0;
}

void CWStringBuffer::reserve(unsigned int capacity)
{
    if (capacity != 0) {
        wchar_t* data = new wchar_t[capacity];
        // Keep as much of the old text as fits (one slot stays for the terminator).
        unsigned int length = m_length;
        if (length > capacity - 1) {
            length = capacity - 1;
        }
        if (length != 0) {
            memcpy(data, m_data, length * sizeof(wchar_t));
        }
        data[length] = 0;
        if (m_data != NULL) {
            delete[] m_data;
        }
        m_data = data;
        m_length = length;
    }
    m_capacity = capacity;
}

void CWStringBuffer::append(const wchar_t* text, unsigned int length)
{
    if (length == 0) {
        return;
    }
    memcpy(m_data + m_length, text, length * sizeof(wchar_t));
    m_length += length;
    m_data[m_length] = 0;
}
