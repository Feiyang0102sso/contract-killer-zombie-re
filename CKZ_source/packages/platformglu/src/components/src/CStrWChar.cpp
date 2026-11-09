// CStrWChar.cpp - out-of-line members of the wide string class.
#include "CStrWChar.h"
#include "CInputStream.h"
#include "CUtf.h"

namespace com { namespace glu { namespace platform { namespace components {

void CStrWChar::ReleaseMemory()
{
    m_length = 0;
    if (m_string != NULL) {
        delete[] m_string;
        m_string = NULL;
    }
}

unsigned char CStrWChar::Load(CInputStream& stream, unsigned int encoding)
{
    ReleaseMemory();

    unsigned char loaded = 0;
    int byteCount = stream.Available();
    if (byteCount == 0) {
        return loaded;
    }

    switch (encoding) {
    case EncodingUtf8: {
        m_string = new wchar_t[byteCount + 1];
        unsigned char* bytes = new unsigned char[byteCount];
        if (m_string != NULL && bytes != NULL) {
            loaded = 1;
            stream.Read(bytes, byteCount);
            core::CUtf::Utf8ToWcs(bytes, byteCount, m_string, byteCount + 1);
            m_length = wcslen(m_string);
        }
        if (bytes != NULL) {
            delete[] bytes;
        }
        break;
    }

    case EncodingWide:
        // the stream holds 2-byte characters
        m_length = byteCount >> 1;
        m_string = new wchar_t[m_length + 1];
        if (m_string != NULL) {
            stream.Read((unsigned char*)m_string, byteCount);
            m_string[m_length] = 0;
            loaded = 1;
        }
        break;
    }
    return loaded;
}

void CStrWChar::Concatenate(const wchar_t* text)
{
    if (text == NULL) {
        return;
    }

    int textLength = wcslen(text);
    int newLength = m_length + textLength;
    wchar_t* buffer = new wchar_t[newLength + 1];
    buffer[0] = 0;
    if (m_string != NULL) {
        wcscpy(buffer, m_string);
        ReleaseMemory();
    }
    m_string = buffer;
    m_length = newLength;
    wcscat(buffer, text);
}

void CStrWChar::Concatenate(const char* text)
{
    if (text == NULL) {
        return;
    }

    int textLength = strlen(text);
    int oldLength = m_length;
    int newLength = oldLength + textLength;
    wchar_t* buffer = new wchar_t[newLength + 1];
    buffer[0] = 0;
    if (m_string != NULL) {
        wcscpy(buffer, m_string);
        ReleaseMemory();
    }
    m_string = buffer;
    m_length = newLength;
    mbstowcs(buffer + oldLength, text, strlen(text) + 1);
}

// The non-trivial branch returns a plain pointer, converted by the matching constructor:
// with more than one returned object clang does not apply NRVO, so the result is a copy.

CStrWChar operator+(const CStrWChar& left, const CStrWChar& right)
{
    const wchar_t* only = NULL;
    if (left.m_string != NULL) {
        if (right.m_string != NULL) {
            CStrWChar joined(left.m_string);
            joined.Concatenate(right.m_string);
            return joined.m_string;
        }
        only = left.m_string;
    } else if (right.m_string != NULL) {
        only = right.m_string;
    }
    return only;
}

CStrWChar operator+(const CStrWChar& left, const wchar_t* right)
{
    if (left.m_string != NULL && right != NULL) {
        CStrWChar joined(left.m_string);
        joined.Concatenate(right);
        return joined.m_string;
    }
    const wchar_t* only = right;
    if (right == NULL || left.m_string != NULL) {
        only = left.m_string;
    }
    return only;
}

CStrWChar operator+(const wchar_t* left, const CStrWChar& right)
{
    const wchar_t* only = NULL;
    if (left != NULL) {
        if (right.m_string != NULL) {
            CStrWChar joined(left);
            joined.Concatenate(right.m_string);
            return joined.m_string;
        }
        only = left;
    } else if (right.m_string != NULL) {
        only = right.m_string;
    }
    return only;
}

CStrWChar operator+(const CStrWChar& left, const char* right)
{
    if (left.m_string != NULL && right != NULL) {
        CStrWChar joined(left.m_string);
        joined.Concatenate(right);
        return joined.m_string;
    }
    if (left.m_string != NULL) {
        return left.m_string;
    }
    if (right != NULL) {
        return right;
    }
    return CStrWChar();
}

/** Blank characters stripped by Trim(). */
static inline bool IsTrimmedChar(wchar_t value)
{
    if (value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f') {
        return true;
    }
    return false;
}

CStrWChar CStrWChar::Trim() const
{
    CStrWChar result;
    if (m_length > 0) {
        int first = 0;
        int last = m_length - 1;
        while (first <= last && IsTrimmedChar(m_string[first])) {
            first++;
        }
        while (first <= last && IsTrimmedChar(m_string[last])) {
            last--;
        }
        DoGetSubstring(result, *this, first, last + 1);
    }
    return result;
}

void CStrWChar::DoGetSubstring(CStrWChar& destination, const CStrWChar& source, int startIndex, int endIndex)
{
    int length = endIndex - startIndex;
    if (length > 0) {
        wchar_t* buffer = new wchar_t[length + 1];
        wcsncpy(buffer, source.m_string + startIndex, length + 1);
        buffer[length] = 0;
        // leftover of a debug length check: the result is unused, but wcslen is not
        // known to be side-effect free, so the call stays in the binary
        wcslen(buffer);
        destination.ReleaseMemory();
        destination.m_string = buffer;
        destination.m_length = length;
    } else {
        destination.ReleaseMemory();
    }
}

int CStrWChar::Find(const wchar_t* text, int startIndex) const
{
    int textLength = wcslen(text);
    if (textLength == 0 && startIndex <= m_length) {
        return startIndex;
    }

    int lastStart = m_length - textLength;
    for (int position = startIndex; position <= lastStart; position++) {
        const wchar_t* candidate = m_string + position;
        int matched = 0;
        while (matched < textLength && text[matched] == candidate[matched]) {
            matched++;
        }
        if (matched >= textLength) {
            return position;
        }
    }
    return -1;
}

int CStrWChar::GetCharIndex(int occurrence, wchar_t character) const
{
    int foundIndex = -1;
    int foundCount = 0;
    int index = 0;
    const wchar_t* end = m_string + m_length;
    for (const wchar_t* cursor = m_string; cursor < end; cursor++) {
        if (*cursor == character) {
            foundCount++;
            if (occurrence == 0 || foundCount == occurrence) {
                foundIndex = index;
            }
            if (occurrence != 0 && foundCount == occurrence) {
                foundIndex = index;
                break;
            }
        }
        index++;
    }
    return foundIndex;
}

CStrWChar CStrWChar::GetSubString(int startIndex, int endIndex) const
{
    CStrWChar result;
    if (m_length < 1) {
        return result;
    }

    if (endIndex < 0) {
        endIndex = 0;
    }
    if (startIndex < 0) {
        startIndex = 0;
    }
    int low = endIndex;
    if (startIndex < endIndex) {
        low = startIndex;
    }
    int high = endIndex;
    if (startIndex > endIndex) {
        high = startIndex;
    }
    DoGetSubstring(result, *this, low, high + 1);

    // a backwards range yields the characters in reverse order
    if (startIndex > endIndex && result.m_length > 0) {
        wchar_t* front = result.m_string;
        wchar_t* back = result.m_string + result.m_length - 1;
        while (front < back) {
            wchar_t swapped = *front;
            *front = *back;
            *back = swapped;
            front++;
            back--;
        }
    }
    return result;
}

} } } }
