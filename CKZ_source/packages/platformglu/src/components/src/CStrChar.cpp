// CStrChar.cpp - out-of-line members of the char string class.
#include "CStrChar.h"

namespace com { namespace glu { namespace platform { namespace components {

void CStrChar::ReleaseMemory()
{
    m_length = 0;
    if (m_string != NULL) {
        delete[] m_string;
        m_string = NULL;
    }
}

void CStrChar::Concatenate(const char* text)
{
    if (text == NULL) {
        return;
    }

    int textLength = strlen(text);
    int newLength = m_length + textLength;
    char* buffer = new char[newLength + 1];
    buffer[0] = '\0';
    if (m_string != NULL) {
        strcpy(buffer, m_string);
        ReleaseMemory();
    }
    m_string = buffer;
    m_length = newLength;
    strcat(buffer, text);
}

// The non-trivial branch returns a plain char pointer, converted by CStrChar(const char*):
// with more than one returned object clang does not apply NRVO, so the result is a copy.

CStrChar operator+(const CStrChar& left, const char* right)
{
    if (left.m_string != NULL && right != NULL) {
        CStrChar joined(left.m_string);
        joined.Concatenate(right);
        return joined.m_string;
    }
    const char* only = right;
    if (right == NULL || left.m_string != NULL) {
        only = left.m_string;
    }
    return only;
}

CStrChar operator+(const char* left, const CStrChar& right)
{
    if (left != NULL && right.m_string != NULL) {
        CStrChar joined(left);
        joined.Concatenate(right.m_string);
        return joined.m_string;
    }
    const char* only = left;
    if (left == NULL) {
        only = right.m_string;
    }
    return only;
}

/** Blank characters stripped by Trim(). */
static inline bool IsTrimmedChar(char value)
{
    if (value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f') {
        return true;
    }
    return false;
}

CStrChar CStrChar::Trim() const
{
    CStrChar result;
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

void CStrChar::DoGetSubstring(CStrChar& destination, const CStrChar& source, int startIndex, int endIndex)
{
    int length = endIndex - startIndex;
    if (length > 0) {
        char* buffer = new char[length + 1];
        strncpy(buffer, source.m_string + startIndex, length + 1);
        buffer[length] = '\0';
        destination.ReleaseMemory();
        destination.m_string = buffer;
        destination.m_length = length;
    } else {
        destination.ReleaseMemory();
    }
}

int CStrChar::Find(const char* text, int startIndex) const
{
    int textLength = strlen(text);
    if (textLength == 0 && startIndex <= m_length) {
        return startIndex;
    }

    int lastStart = m_length - textLength;
    for (int position = startIndex; position <= lastStart; position++) {
        const char* candidate = m_string + position;
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

CStrChar CStrChar::GetSubString(int startIndex, int endIndex) const
{
    CStrChar result;
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
        char* front = result.m_string;
        char* back = result.m_string + result.m_length - 1;
        while (front < back) {
            char swapped = *front;
            *front = *back;
            *back = swapped;
            front++;
            back--;
        }
    }
    return result;
}

} } } }
