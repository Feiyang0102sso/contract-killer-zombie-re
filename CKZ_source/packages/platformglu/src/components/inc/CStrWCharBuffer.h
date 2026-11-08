// CStrWCharBuffer.h - growable wide string builder (created by b006).
// Wide counterpart of CStrCharBuffer: spare capacity, appends/inserts/replacements in place.
#ifndef CSTRWCHARBUFFER_H
#define CSTRWCHARBUFFER_H

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CStrWCharBuffer : public core::CClass {
public:
    /** Runtime class id: CStringToKey("CStrWCharBuffer"). */
    static const unsigned int ClassId = 0xd010737c;
    /** Capacity of a new buffer, in characters (excluding the terminator). */
    static const int DefaultCapacity = 15;

    CStrWCharBuffer()
    {
        m_classId = ClassId;
        m_capacity = DefaultCapacity;
        m_string = new wchar_t[DefaultCapacity + 1];
        m_length = 0;
    }

    /** Buffer able to hold `capacity` characters without growing. */  // added by b033x
    inline explicit CStrWCharBuffer(int capacity)  // added by b033x
    {  // added by b033x
        m_classId = ClassId;  // added by b033x
        m_capacity = capacity;  // added by b033x
        m_string = new wchar_t[capacity + 1];  // added by b033x
        m_length = 0;  // added by b033x
    }  // added by b033x

    /** Buffer starting with `text`. */  // added by b023x
    inline explicit CStrWCharBuffer(const wchar_t* text)  // added by b023x
    {  // added by b023x
        m_classId = ClassId;  // added by b023x
        m_capacity = DefaultCapacity;  // added by b023x
        m_string = new wchar_t[DefaultCapacity + 1];  // added by b023x
        m_length = 0;  // added by b023x
        Append(text);  // added by b023x
    }  // added by b023x

    virtual ~CStrWCharBuffer();

    /** Contents (always allocated; NUL-terminated after the first change). */
    const wchar_t* GetWCharPtr() const
    {
        return m_string;
    }

    int GetLength() const
    {
        return m_length;
    }

    /** Grow the buffer so it holds at least `capacity` characters. */
    void EnsureCapacity(int capacity);
    CStrWCharBuffer& Append(const wchar_t* text);
    /** Append `count` characters of `text` starting at `start` (the whole `text` must be NUL-terminated). */
    CStrWCharBuffer& Append(const wchar_t* text, int start, unsigned long count);
    /** Insert a char string (widened with mbstowcs) at character `position`. */
    CStrWCharBuffer& Insert(int position, const char* text);
    /** Insert the decimal form of `value` at character `position`. */
    CStrWCharBuffer& Insert(int position, int value);
    /**
     * Replace every occurrence of `pattern[patternStart, patternStart + patternLength)` inside
     * this[searchStart, searchStart + searchLength) by `replacement`.
     */
    CStrWCharBuffer& Replace(const wchar_t* pattern, const wchar_t* replacement, int patternStart,
                             unsigned long patternLength, int searchStart, unsigned long searchLength);

    wchar_t* m_string;  // 0x08 buffer of m_capacity + 1 characters
    int m_length;       // 0x0c characters in use
    int m_capacity;     // 0x10 characters the buffer can hold
};

// defined out of the class body: the declaration above makes it the key function
inline CStrWCharBuffer::~CStrWCharBuffer()
{
    if (m_string != NULL) {
        delete[] m_string;
        m_string = NULL;
    }
    m_length = 0;
    m_capacity = 0;
}

} } } }

#endif
