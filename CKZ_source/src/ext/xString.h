// xString.h - reference counted wide string of the game layer (created by b051x).
// The string keeps a pointer to its characters; a Data header (reference count, size in bytes)
// sits right before them. All empty strings share the characters of XString::pEmpty.
#ifndef XSTRING_H
#define XSTRING_H

#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include "CStdUtil_Cocoa.h"
#include "Vector_ss.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class CFont;
} } } }

/** swprintf into `buffer`; `size` is not checked. Variadic, so every user keeps a weak copy. */
inline int __wsprintf(wchar_t* buffer, int size, const wchar_t* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = CStdUtil_Cocoa::VSWPrintF(buffer, format, arguments);
    va_end(arguments);
    return written;
}

class XString {
public:
    /** Header stored right before the characters. */
    struct Data {
        int m_refCount;     // 0x00
        int m_byteSize;     // 0x04 size of the characters in bytes (without the terminator)

        /** New block with room for `bytes` (plus zeroed terminator space); NULL when out of memory. */
        static Data* Alloc(int bytes, int reserveBytes);
        /** Drop one reference; the block is freed with the last one. */
        void Release();

        wchar_t* Chars()
        {
            return (wchar_t*)(this + 1);
        }
    };

    /** char* copy of a wide string (wcstombs), owned by this object. */
    class AnsiString {
    public:
        AnsiString(const wchar_t* text);
        ~AnsiString()
        {
            if (m_text != NULL) {
                free(m_text);
                m_text = NULL;
            }
        }

        operator const char*() const
        {
            return m_text;
        }

        char* m_text;   // 0x00
    };

    XString()
    {
        InitEmpty();
    }

    XString(const XString& other)
    {
        m_data = other.m_data;
        GetData()->m_refCount++;
    }

    XString(const char* text)  // added by b057x
    {  // added by b057x
        Init(text);  // added by b057x
    }  // added by b057x

    XString(const wchar_t* text)
    {
        Init(text, wcslen(text) * sizeof(wchar_t));
    }

    XString(const void* bytes, int byteCount)
    {
        Init(bytes, byteCount);
    }

    XString(const void* first, int firstBytes, const void* second, int secondBytes)  // added by b048
    {  // added by b048
        Init(first, firstBytes, second, secondBytes);  // added by b048
    }  // added by b048

    XString(const void* first, int firstBytes, const void* second, int secondBytes, const void* third, int thirdBytes)
    {
        Init(first, firstBytes, second, secondBytes, third, thirdBytes);
    }

    XString(int value);
    XString(float value, int decimals);

    inline ~XString()
    {
        GetData()->Release();
    }

    XString& operator=(const XString& other)
    {
        return Assign(other);
    }

    XString& operator=(const wchar_t* text)  // added by b048
    {  // added by b048
        return Assign(text, wcslen(text) * sizeof(wchar_t));  // added by b048
    }  // added by b048

    /** swprintf into a new string (defined in src/CGameApp.cpp). */  // added by b048
    // b063: the preceding definition location was provisional; 0x0e0780 is a real weak inline body.
    static inline XString Format(const wchar_t* format, ...)  // added by b048
    {
        va_list arguments;
        va_start(arguments, format);
        XString result;
        XString formatted = result.SetFormat(format, arguments);
        va_end(arguments);
        return formatted;
    }

    Data* GetData() const
    {
        return (Data*)m_data - 1;
    }

    /** Length in characters. */
    int Length() const
    {
        return GetData()->m_byteSize / sizeof(wchar_t);
    }

    /** Size of the characters in bytes. */
    int ByteSize() const
    {
        return GetData()->m_byteSize;
    }

    const wchar_t* c_str() const
    {
        return m_data;
    }

    /** Share the characters of the empty string. */
    void InitEmpty()
    {
        m_data = pEmpty->m_data;
        GetData()->m_refCount++;
    }

    /** Create / destroy the shared empty string. */
    static void Init();
    static void Shutdown();

    void Init(const char* text, int length);
    void Init(const void* bytes, int byteCount);
    void Init(const void* first, int firstBytes, const void* second, int secondBytes);
    void Init(const void* first, int firstBytes, const void* second, int secondBytes, const void* third, int thirdBytes);
    void Init(const char* text);

    /** Format `value` with `decimals` digits after the point (negative: round to 10^-decimals). */
    static wchar_t* ToString(wchar_t* buffer, float value, int decimals);
    static XString ToString(int value);

    /** Resize to `byteCount` bytes; keep the old characters when `keepContent`. */
    XString& SetSize(int byteCount, int reserveBytes, bool keepContent);
    XString& Assign(const XString& other);
    XString& Assign(const void* bytes, int byteCount);
    XString& SetFormat(const wchar_t* format, void* arguments);

    /** Break into lines that fit `width` pixels with `font`. */
    Vector<XString>& Split(Vector<XString>& lines, int width, com::glu::platform::graphics::CFont* font) const;
    /** Break at '\n' / '\r' (and their pairs); every line is trimmed. */
    Vector<XString>& SplitLines(Vector<XString>& lines) const;
    Vector<XString> Split(const wchar_t* separator) const;
    void Split(Vector<XString>& parts, const wchar_t* separator) const;

    XString Trim(bool left, bool right) const;
    XString SubString(int start, int count) const;
    XString SubData(int startByte, int byteCount) const;
    XString Replace(const wchar_t* from, const wchar_t* to, bool all) const;
    XString Lower() const;
    XString Upper() const;

    /** Byte offset of `bytes` at or after `startByte`, -1 when absent. */
    int FindData(const void* bytes, int byteCount, int startByte) const;
    int Find(const wchar_t* text, int start) const;
    int Find(const XString& text, int start) const;

    wchar_t Char(int index) const;
    XString& SetChar(int index, wchar_t character);
    XString& InsertData(int startByte, const void* bytes, int byteCount);
    XString& Append(const XString& other);
    XString& Append(const void* bytes, int byteCount);

    static int CmpData(const void* first, int firstBytes, const void* second, int secondBytes, int maxBytes);
    int Cmp(const XString& other, int maxLength) const;

    /** Decimal digits at `cursor` (advanced past them); true when at least one was read. */
    static bool ParseDecSimple(const wchar_t*& cursor, int& value);
    /** Number at `cursor`: 0 = none, 1 = floating point, 2 = integer. */
    static int ParseNum(const wchar_t*& cursor, double& value);
    float ToFloat() const;
    int ToInt() const;
    bool ToBool() const;

    /** djb2 hash of `byteCount` bytes. */
    static int HashData(const void* bytes, int byteCount);

    wchar_t* m_data;            // 0x00 characters (Data header right before)

    static XString* pEmpty;     // shared empty string
};

/** Concatenation. */  // added by b048
inline XString operator+(const XString& first, const XString& second)  // added by b048
{  // added by b048
    return XString(first.m_data, first.ByteSize(), second.m_data, second.ByteSize());  // added by b048
}  // added by b048

inline XString operator+(const XString& first, const wchar_t* second)  // added by b048
{  // added by b048
    return XString(first.m_data, first.ByteSize(), second, wcslen(second) * sizeof(wchar_t));  // added by b048
}  // added by b048

/** Wide literal prefix plus a string; CWaveManager::Parse at 0x14068c uses one combined Init. */  // added by b080
inline XString operator+(const wchar_t* first, const XString& second)  // added by b080
{
    return XString(first, wcslen(first) * sizeof(wchar_t), second.m_data, second.ByteSize());
}

/** Same characters (CmpData over the whole strings). */  // added by b077
inline bool operator==(const XString& first, const XString& second)  // added by b077
{  // added by b077
    return XString::CmpData(first.m_data, first.ByteSize(), second.m_data, second.ByteSize(), 0x7fffffff) == 0;  // added by b077
}  // added by b077

/** Compare wide characters directly; CPowerUp @0x12fc94 and IAP @0x12ab80. */  // added by b078a
inline bool operator==(const XString& first, const wchar_t* second)
{
    return XString::CmpData(first.m_data, first.ByteSize(), second, wcslen(second) * sizeof(wchar_t), 0x7fffffff) == 0;
}

#endif
