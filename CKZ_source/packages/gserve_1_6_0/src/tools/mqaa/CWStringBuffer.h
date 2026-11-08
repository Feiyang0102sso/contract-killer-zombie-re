// Owner: tools/mqaa/CWStringBuffer.cpp (b028x; started as a b020 PROVISIONAL)
// CWStringBuffer.h - growable, always NUL-terminated wide string builder used by the JSON encoder.
// reserve(), append(text, length) and the destructor live in CWStringBuffer.cpp; the appends that
// check the capacity first are inline (as inlined into CNGSJSONParser.cpp).
#ifndef CWSTRINGBUFFER_H
#define CWSTRINGBUFFER_H

#include <stddef.h>
#include <wchar.h>

class CWStringBuffer {
public:
    CWStringBuffer()
        : m_data(NULL)
        , m_length(0)
        , m_capacity(0)
    {
    }

    /** Empty buffer with room for `capacity` characters (terminator included). */
    inline explicit CWStringBuffer(unsigned int capacity)
        : m_data(NULL)
        , m_length(0)
        , m_capacity(0)
    {
        reserve(capacity);
    }

    /** Buffer holding a copy of `text`, with room for as much again. */
    inline explicit CWStringBuffer(const wchar_t* text)
        : m_data(NULL)
        , m_length(0)
        , m_capacity(0)
    {
        unsigned int length = wcslen(text);
        reserve(length * 2 + 1);
        append(text, length);
    }

    ~CWStringBuffer();

    /** Resize the storage to `capacity` characters (the text is cut to fit). */
    void reserve(unsigned int capacity);
    /** Append `length` characters without checking the capacity. */
    void append(const wchar_t* text, unsigned int length);

    /** Append one character, growing to at least twice the capacity when full. */
    inline void append(wchar_t character)
    {
        unsigned int required = m_length + 2;
        if (required > m_capacity) {
            if (m_capacity * 2 > required) {
                required = m_capacity * 2;
            }
            reserve(required);
        }
        m_data[m_length] = character;
        m_length++;
        m_data[m_length] = 0;
    }

    /** Append a NUL-terminated string (NULL is ignored), growing to at least twice the capacity. */
    inline void append(const wchar_t* text)
    {
        if (text == NULL) {
            return;
        }
        unsigned int length = wcslen(text);
        if (length > m_capacity - 1 - m_length) {
            unsigned int required = m_length + length + 1;
            if (m_capacity * 2 > required) {
                required = m_capacity * 2;
            }
            reserve(required);
        }
        append(text, length);
    }

    const wchar_t* getBuffer() const
    {
        return m_data;
    }

    wchar_t* m_data;            // +0x00 NUL-terminated text (NULL until the first reserve)
    unsigned int m_length;      // +0x04 characters, not counting the terminator
    unsigned int m_capacity;    // +0x08 characters allocated
};

#endif
