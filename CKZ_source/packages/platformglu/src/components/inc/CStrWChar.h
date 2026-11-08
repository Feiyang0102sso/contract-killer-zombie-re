// CStrWChar.h - heap-allocated, NUL-terminated wide string (created by b006).
// Same storage rules as CStrChar: exact-size buffer rebuilt on every change, NULL means empty.
#ifndef CSTRWCHAR_H
#define CSTRWCHAR_H

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "CClass.h"

namespace com { namespace glu { namespace platform { namespace components {

class CInputStream;

class CStrWChar : public core::CClass {
public:
    /** Runtime class id: CStringToKey("CStrWChar"). */
    static const unsigned int ClassId = 0x43735eb4;

    /** Load() encodings: raw wchar_t data (the CStrWChar class id) or UTF-8 bytes. */
    static const unsigned int EncodingWide = 0x43735eb4;
    static const unsigned int EncodingUtf8 = 0xf686aadc;

    CStrWChar()
    {
        m_classId = ClassId;
        m_string = NULL;
        m_length = 0;
    }

    CStrWChar(const wchar_t* text)
    {
        m_classId = ClassId;
        m_length = 0;
        m_string = NULL;
        Concatenate(text);
    }

    /** Widen a char string (mbstowcs). */
    CStrWChar(const char* text)
    {
        m_classId = ClassId;
        m_length = 0;           // original stores the length before the pointer (CAppInfo ctor, b005)
        m_string = NULL;
        Concatenate(text);
    }

    inline CStrWChar(const CStrWChar& other)
    {
        m_classId = ClassId;
        m_length = 0;           // length before pointer, as in the original (b003)
        m_string = NULL;
        Concatenate(other.m_string);
    }

    virtual ~CStrWChar()
    {
        ReleaseMemory();
    }

    inline CStrWChar& operator=(const wchar_t* text)
    {
        if (m_string != text) {
            ReleaseMemory();
            Concatenate(text);
        }
        return *this;
    }

    inline CStrWChar& operator=(const CStrWChar& other)
    {
        if (other.m_string != m_string) {
            ReleaseMemory();
            Concatenate(other.m_string);
        }
        return *this;
    }

    /** Assign a char string; never aliases the wide buffer, so no self-check. */
    inline CStrWChar& operator=(const char* text)
    {
        ReleaseMemory();
        Concatenate(text);
        return *this;
    }

    CStrWChar& operator+=(const wchar_t* text)
    {
        Concatenate(text);
        return *this;
    }

    CStrWChar& operator+=(const CStrWChar& other)
    {
        Concatenate(other.m_string);
        return *this;
    }

    CStrWChar& operator+=(const char* text)
    {
        Concatenate(text);
        return *this;
    }

    /** Equal contents; NULL only equals NULL. */
    inline bool operator==(const wchar_t* text) const
    {
        // see operator==(const CStrWChar&) (fx1)
        const wchar_t* left = m_string;
        const wchar_t* right = text;
        if (left != NULL && right != NULL) {
            return wcscmp(left, right) == 0;
        }
        return left == right;
    }

    /** Equal contents; two NULL buffers are equal. */
    inline bool operator==(const CStrWChar& other) const
    {
        // both buffers in locals first: the original tests them with one merged branch
        // (cmp a,#0; it ne; cmpne b,#0) (fx1)
        const wchar_t* left = m_string;
        const wchar_t* right = other.m_string;
        if (left != NULL && right != NULL) {
            return wcscmp(left, right) == 0;
        }
        return left == right;
    }

    /** Raw buffer (NULL while the string has never been assigned). */
    const wchar_t* GetWCharPtr() const
    {
        return m_string;
    }

    int GetLength() const
    {
        return m_length;
    }

    /** Free the buffer; the string becomes empty (NULL buffer). */
    void ReleaseMemory();
    /**
     * Replace the contents with everything left in `stream`, decoded per `encoding`
     * (EncodingWide or EncodingUtf8). Returns 1 on success.
     */
    unsigned char Load(CInputStream& stream, unsigned int encoding);
    /** Append `text` (NULL is ignored). */
    void Concatenate(const wchar_t* text);
    /** Append a char string widened with mbstowcs (NULL is ignored). */
    void Concatenate(const char* text);

    /** Copy with leading/trailing blanks (space, \t, \n, \f, \r) removed. */
    CStrWChar Trim() const;
    /** Index of the first `text` at or after `startIndex`, -1 if absent. */
    int Find(const wchar_t* text, int startIndex) const;
    /** Index of the `occurrence`-th `character` (1-based; 0 = last one), -1 if absent. */
    int GetCharIndex(int occurrence, wchar_t character) const;
    /** Characters [min(a,b), max(a,b)]; reversed when `startIndex` > `endIndex`. */
    CStrWChar GetSubString(int startIndex, int endIndex) const;

    /** destination = source[startIndex, endIndex) (empty when the range is empty). */
    static void DoGetSubstring(CStrWChar& destination, const CStrWChar& source, int startIndex, int endIndex);

    wchar_t* m_string;  // 0x08 NUL-terminated buffer or NULL
    int m_length;       // 0x0c characters, not counting the terminator
};

CStrWChar operator+(const CStrWChar& left, const CStrWChar& right);
CStrWChar operator+(const CStrWChar& left, const wchar_t* right);
CStrWChar operator+(const wchar_t* left, const CStrWChar& right);
CStrWChar operator+(const CStrWChar& left, const char* right);

} } } }

#endif
