// CStrCharBuffer.h - growable char string builder (created by b006).
// Unlike CStrChar the buffer keeps spare capacity, so appends do not reallocate every time.
#ifndef CSTRCHARBUFFER_H
#define CSTRCHARBUFFER_H

#include <string.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CStrCharBuffer : public core::CClass {
public:
    /**
     * Runtime class id. Note: this is not CStringToKey("CStrCharBuffer") (0x6c37367c);
     * the original source uses this value.
     */
    static const unsigned int ClassId = 0xd37367e6;
    /** Capacity of a new buffer, in characters (excluding the terminator). */
    static const int DefaultCapacity = 15;

    CStrCharBuffer()
    {
        m_classId = ClassId;
        m_capacity = DefaultCapacity;
        m_string = new char[DefaultCapacity + 1];
        m_length = 0;
    }

    virtual ~CStrCharBuffer();

    /** Contents (always allocated; NUL-terminated after the first append). */
    const char* GetCharPtr() const
    {
        return m_string;
    }

    int GetLength() const
    {
        return m_length;
    }

    /** Grow the buffer so it holds at least `capacity` characters. */
    void EnsureCapacity(int capacity);
    CStrCharBuffer& Append(const char* text);

    /** Append one character. */  // added by b040x
    inline void Append(char character)  // added by b040x
    {  // added by b040x
        EnsureCapacity(m_length + 1);  // added by b040x
        m_string[m_length++] = character;  // added by b040x
        m_string[m_length] = 0;  // added by b040x
    }  // added by b040x

    char* m_string;     // 0x08 buffer of m_capacity + 1 chars
    int m_length;       // 0x0c characters in use
    int m_capacity;     // 0x10 characters the buffer can hold
};

// defined out of the class body: the declaration above makes it the key function, so every
// file including this header emits the (weak) vtable and destructors
inline CStrCharBuffer::~CStrCharBuffer()
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
