// CString.h - zero-terminated character vector used by the game's notification JSON reader.
// The original header path is not preserved in N_SOL; the global template name is preserved.
#ifndef GAME_CSTRING_H
#define GAME_CSTRING_H

#include "CVector.h"

template <class T>
class CString {
public:
    CString()
    {
        T zero = 0;
        m_characters.Add(zero);
    }

    CString(const T* text)
    {
        while (*text != 0) {
            m_characters.Add(*text);
            text++;
        }
        T zero = 0;
        m_characters.Add(zero);
    }

    /** The terminator is a vector element, so the empty string has Size() == 1. */
    int Length() const { return m_characters.m_size - 1; }
    T& operator[](int index) { return m_characters[index]; }
    const T& operator[](int index) const { return m_characters[index]; }
    void RemoveRange(int start, int end) { m_characters.RemoveRange(start, end); }

    void Append(T character)
    {
        T zero = 0;
        m_characters.Add(zero);
        m_characters.m_data[m_characters.m_size - 2] = character;
    }

    void Trim(T character)
    {
        while (m_characters.m_size != 1 && m_characters.m_data[0] == character) {
            m_characters.RemoveRange(0, 1);
        }
        // Original 0xe0778 requires at least two non-terminator characters at this end.
        while (m_characters.m_size - 1 >= 2) {
            int last = m_characters.m_size - 2;
            if (m_characters.m_data[last] != character) {
                return;
            }
            m_characters.RemoveRange(last, last + 1);
        }
    }

    // Composition preserves the original CVector vptr; CString has no vtable of its own.
    com::glu::platform::core::CVector<T> m_characters;  // 0x00
};

#endif
