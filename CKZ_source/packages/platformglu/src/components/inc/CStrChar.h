// CStrChar.h - heap-allocated, NUL-terminated char string (created by b006).
// The buffer is reallocated on every change: Concatenate() builds a new block of exactly
// length + 1 chars. A NULL buffer means "empty"; GetCharPtr() may therefore return NULL.
#ifndef CSTRCHAR_H
#define CSTRCHAR_H

#include <string.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CStrChar : public core::CClass {
public:
    /** Runtime class id: CStringToKey("CStrChar"). */
    static const unsigned int ClassId = 0x64365e6e;

    CStrChar()
    {
        m_classId = ClassId;
        m_string = NULL;
        m_length = 0;
    }

    CStrChar(const char* text)
    {
        m_classId = ClassId;
        m_length = 0;
        m_string = NULL;
        Concatenate(text);
    }

    CStrChar(const CStrChar& other)
    {
        m_classId = ClassId;
        m_length = 0;           // original stores the length before the pointer (CHttpTransport::Cleanup, b000x)
        m_string = NULL;
        Concatenate(other.m_string);
    }

    virtual ~CStrChar();

    inline CStrChar& operator=(const char* text)
    {
        if (m_string != text) {
            ReleaseMemory();
            Concatenate(text);
        }
        return *this;
    }

    inline CStrChar& operator=(const CStrChar& other)
    {
        if (other.m_string != m_string) {
            ReleaseMemory();
            Concatenate(other.m_string);
        }
        return *this;
    }

    CStrChar& operator+=(const char* text)
    {
        Concatenate(text);
        return *this;
    }

    CStrChar& operator+=(const CStrChar& other)
    {
        Concatenate(other.m_string);
        return *this;
    }

    /** Equal contents; two NULL buffers are equal, NULL never equals a real string. */
    inline bool operator==(const CStrChar& other) const
    {
        // both buffers in locals first: the original tests them with one merged branch
        // (cmp a,#0; it ne; cmpne b,#0) (fx1)
        const char* left = m_string;
        const char* right = other.m_string;
        if (left != NULL && right != NULL) {
            return strcmp(left, right) == 0;
        }
        return left == right;
    }

    /** Different contents (same NULL rules as operator==). */  // added by b000x
    inline bool operator!=(const CStrChar& other) const  // added by b000x
    {  // added by b000x
        // fx1: both buffers in locals first (merged null test, see operator==)
        const char* left = m_string;
        const char* right = other.m_string;
        bool different;  // added by b000x
        if (left != NULL && right != NULL) {  // added by b000x
            different = strcmp(left, right) != 0;  // added by b000x
        } else {  // added by b000x
            different = left != right;  // added by b000x
        }  // added by b000x
        return different;  // added by b000x
    }  // added by b000x

    /** Raw buffer (NULL while the string has never been assigned). */
    const char* GetCharPtr() const
    {
        return m_string;
    }

    int GetLength() const
    {
        return m_length;
    }

    /** Free the buffer; the string becomes empty (NULL buffer). */
    void ReleaseMemory();
    /** Append `text` (NULL is ignored). */
    void Concatenate(const char* text);

    /** Copy with leading/trailing blanks (space, \t, \n, \f, \r) removed. */
    CStrChar Trim() const;
    /** Index of the first `text` at or after `startIndex`, -1 if absent. */
    int Find(const char* text, int startIndex) const;
    /** Characters [min(a,b), max(a,b)]; reversed when `startIndex` > `endIndex`. */
    CStrChar GetSubString(int startIndex, int endIndex) const;

    /** destination = source[startIndex, endIndex) (empty when the range is empty). */
    static void DoGetSubstring(CStrChar& destination, const CStrChar& source, int startIndex, int endIndex);

    char* m_string;     // 0x08 NUL-terminated buffer or NULL
    int m_length;       // 0x0c characters, not counting the terminator
};

// The first original ObjC++ unit contains a weak SjLj destructor copy; uses the same out-of-class inline as CRenderSurfaceBuffer.
inline CStrChar::~CStrChar()
{
    ReleaseMemory();
}

CStrChar operator+(const CStrChar& left, const char* right);
CStrChar operator+(const char* left, const CStrChar& right);

} } } }

#endif
