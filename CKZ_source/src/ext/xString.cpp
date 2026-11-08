// xString.cpp - reference counted wide string of the game layer.
#include "xString.h"
#include <stdlib.h>
#include "CStdUtil_Cocoa.h"
#include "CTextParser.h"

// Bytes allocated after the characters (zeroed, so the string is always terminated).
#define XSTRING_TAIL_BYTES 6

XString* XString::pEmpty;

XString::Data* XString::Data::Alloc(int bytes, int reserveBytes)
{
    // reserveBytes is not used: blocks are always allocated to the exact size.
    Data* data = (Data*)malloc(bytes + sizeof(Data) + XSTRING_TAIL_BYTES);
    if (data != NULL) {
        data->m_refCount = 1;
        data->m_byteSize = bytes;
        memset(data + 1, 0, bytes + XSTRING_TAIL_BYTES);
    }
    return data;
}

void XString::Data::Release()
{
    m_refCount--;
    if (m_refCount == 0) {
        delete this;
    }
}

void XString::Init()
{
    // Any non empty literal forces a real block; it is then turned into "".
    XString* empty = new XString(L"1");
    pEmpty = empty;
    wchar_t* chars = empty->m_data;
    ((Data*)chars - 1)->m_byteSize = 0;
    chars[0] = 0;
}

void XString::Shutdown()
{
    if (pEmpty == NULL) {
        return;
    }
    delete pEmpty;
    pEmpty = NULL;
}

void XString::Init(const char* text, int length)
{
    Data* data = Data::Alloc(length * sizeof(wchar_t), 0);
    if (data == NULL) {
        InitEmpty();
    } else {
        wchar_t* chars = data->Chars();
        m_data = chars;
        mbstowcs(chars, text, length + 1);
        chars[length] = 0;
        // The original measures the converted string and drops the result.
        wcslen(chars);
    }
}

XString::XString(int value)
{
    wchar_t buffer[24];
    __wsprintf(buffer, 24, L"%d", value);
    Init(buffer, wcslen(buffer) * sizeof(wchar_t));
}

XString::XString(float value, int decimals)
{
    wchar_t buffer[64];
    ToString(buffer, value, decimals);
    Init(buffer, wcslen(buffer) * sizeof(wchar_t));
}

wchar_t* XString::ToString(wchar_t* buffer, float value, int decimals)
{
    if (decimals > 0) {
        int scale = 10;
        for (int digit = 1; digit < decimals; digit++) {
            scale = scale * 10;
        }

        // "-%d.%0<decimals>d"; the leading '-' is skipped for positive values.
        wchar_t format[32] = L"-%d.%0";
        int length = wcslen(format);
        if (decimals < 10) {
            format[length] = L'0' + decimals;
            format[length + 1] = L'd';
            format[length + 2] = 0;
        } else {
            __wsprintf(format + length, 32, L"%dd", decimals);
        }

        const wchar_t* usedFormat = format + 1;
        if (value < 0) {
            value = -value;
            usedFormat = format;
        }
        int fixed = (int)(value * scale + 0.5f);
        __wsprintf(buffer, 64, usedFormat, fixed / scale, fixed % scale);
    } else if (decimals < 0) {
        int scale = 10;
        for (int digit = 1; digit < -decimals; digit++) {
            scale = scale * 10;
        }
        __wsprintf(buffer, 64, L"%d", (int)(value / scale + 0.5f) * scale);
    } else {
        __wsprintf(buffer, 64, L"%d", (int)(value + 0.5f));
    }
    return buffer;
}

void XString::Init(const void* bytes, int byteCount)
{
    if (byteCount <= 0) {
        InitEmpty();
        return;
    }
    Data* data = Data::Alloc(byteCount, 0);
    if (data == NULL) {
        InitEmpty();
        return;
    }
    m_data = data->Chars();
    memcpy(m_data, bytes, byteCount);
}

void XString::Init(const void* first, int firstBytes, const void* second, int secondBytes)
{
    if (firstBytes <= 0) {
        Init(second, secondBytes);
        return;
    }
    if (secondBytes <= 0) {
        Init(first, firstBytes);
        return;
    }
    Data* data = Data::Alloc(firstBytes + secondBytes, 0);
    if (data == NULL) {
        InitEmpty();
        return;
    }
    m_data = data->Chars();
    memcpy(m_data, first, firstBytes);
    memcpy((char*)m_data + firstBytes, second, secondBytes);
}

void XString::Init(const void* first, int firstBytes, const void* second, int secondBytes, const void* third, int thirdBytes)
{
    if (firstBytes <= 0) {
        Init(second, secondBytes, third, thirdBytes);
        return;
    }
    if (secondBytes <= 0) {
        Init(first, firstBytes, third, thirdBytes);
        return;
    }
    if (thirdBytes <= 0) {
        Init(first, firstBytes, second, secondBytes);
        return;
    }
    Data* data = Data::Alloc(firstBytes + secondBytes + thirdBytes, 0);
    if (data == NULL) {
        InitEmpty();
        return;
    }
    m_data = data->Chars();
    memcpy(m_data, first, firstBytes);
    memcpy((char*)m_data + firstBytes, second, secondBytes);
    memcpy((char*)m_data + firstBytes + secondBytes, third, thirdBytes);
}

void XString::Init(const char* text)
{
    Init(text, strlen(text));
}

XString& XString::SetSize(int byteCount, int reserveBytes, bool keepContent)
{
    if (byteCount <= 0 && reserveBytes <= 0) {
        Assign(*pEmpty);
        return *this;
    }

    wchar_t* oldChars = m_data;
    Data* oldData = GetData();
    if (oldData->m_refCount == 1 && oldData->m_byteSize >= byteCount) {
        // Shrink in place: clear the dropped bytes and the terminator space.
        int kept = oldData->m_byteSize;
        if (kept > byteCount) {
            kept = byteCount;
        }
        memset((char*)oldChars + kept, 0, oldData->m_byteSize + 4 - kept);
        oldData->m_byteSize = byteCount;
        return *this;
    }

    if (!keepContent) {
        oldData->Release();
        Data* data = Data::Alloc(byteCount, reserveBytes);
        if (data == NULL) {
            InitEmpty();
        } else {
            m_data = data->Chars();
        }
        return *this;
    }

    Data* data = Data::Alloc(byteCount, reserveBytes);
    if (data == NULL) {
        oldData->Release();
        InitEmpty();
        return *this;
    }
    m_data = data->Chars();
    int copied = byteCount;
    if (oldData->m_byteSize < copied) {
        copied = oldData->m_byteSize;
    }
    memcpy(m_data, oldChars, copied);
    oldData->Release();
    return *this;
}

XString& XString::Assign(const XString& other)
{
    Data* oldData = GetData();
    m_data = other.m_data;
    GetData()->m_refCount++;
    oldData->Release();
    return *this;
}

Vector<XString>& XString::Split(Vector<XString>& lines, int width, com::glu::platform::graphics::CFont* font) const
{
    lines.setSize(0);
    lines.trimToSize();

    const wchar_t* text = m_data;
    com::glu::platform::graphics::CTextParser parser;
    parser.m_font = font;
    parser.m_text = text;
    parser.m_width = width;
    parser.Parse();

    int lineCount = parser.GetNumLines();
    for (int line = 0; line < lineCount; line++) {
        int first = parser.GetIndexForLine(line);
        int count = parser.GetNumCharsForLine(line);
        lines.addElement(XString(text + first, count * sizeof(wchar_t)));
    }
    return lines;
}

Vector<XString>& XString::SplitLines(Vector<XString>& lines) const
{
    lines.setSize(0);
    lines.trimToSize();

    const wchar_t* text = m_data;
    int length = Length();
    int start = 0;
    int index = 0;
    if (index == length || text[0] == 0) {
        return lines;
    }
    do {
        if (text[index] == L'\n' || text[index] == L'\r') {
            lines.addElement(XString(text + start, (index - start) * sizeof(wchar_t)).Trim(true, true));
            // "\r\n" and "\n\r" count as one break.
            if (text[index + 1] == L"\r\n"[text[index] == L'\r']) {
                index++;
            }
            start = index + 1;
        }
        index++;
    } while (index < length && text[index] != 0);
    if (index > start) {
        lines.addElement(SubData(start * sizeof(wchar_t), (index - start) * sizeof(wchar_t)).Trim(true, true));
    }
    return lines;
}

XString XString::Trim(bool left, bool right) const
{
    const wchar_t* begin = m_data;
    const wchar_t* end = m_data + Length();
    bool trimmed = false;
    if (left) {
        while (end > begin && *begin <= L' ') {
            begin++;
            trimmed = true;
        }
    }
    if (right) {
        while (end > begin && end[-1] <= L' ') {
            end--;
            trimmed = true;
        }
    }
    if (trimmed) {
        return XString(begin, (end - begin) * sizeof(wchar_t));
    }
    return *this;
}

XString XString::SubString(int start, int count) const
{
    return SubData(start * sizeof(wchar_t), count * sizeof(wchar_t));
}

XString& XString::SetFormat(const wchar_t* format, void* arguments)
{
    Assign(NULL, 0);
    wchar_t* buffer = (wchar_t*)malloc(0xa000);
    if (buffer != NULL) {
        CStdUtil_Cocoa::VSWPrintF(buffer, format, arguments);
        Assign(buffer, wcslen(buffer) * sizeof(wchar_t));
        free(buffer);
    }
    return *this;
}

XString& XString::Assign(const void* bytes, int byteCount)
{
    Data* oldData = GetData();
    Init(bytes, byteCount);
    oldData->Release();
    return *this;
}

int XString::FindData(const void* bytes, int byteCount, int startByte) const
{
    const char* chars = (const char*)m_data;
    int last = ByteSize() - byteCount;
    for (int position = startByte; position <= last; position++) {
        if (memcmp(chars + position, bytes, byteCount) == 0) {
            return position;
        }
    }
    return -1;
}

int XString::Find(const wchar_t* text, int start) const
{
    const wchar_t* chars = m_data;
    int textLength = wcslen(text);
    int last = Length() - textLength;
    for (int position = start; position < last; position++) {
        if (memcmp(chars + position, text, textLength * sizeof(wchar_t)) == 0) {
            return position;
        }
    }
    return -1;
}

int XString::Find(const XString& text, int start) const
{
    const wchar_t* textChars = text.m_data;
    const wchar_t* chars = m_data;
    int textBytes = text.ByteSize();
    int last = Length() - text.Length();
    for (int position = start; position < last; position++) {
        if (memcmp(chars + position, textChars, textBytes) == 0) {
            return position;
        }
    }
    return -1;
}

Vector<XString> XString::Split(const wchar_t* separator) const
{
    Vector<XString> parts;
    Split(parts, separator);
    return parts;
}

void XString::Split(Vector<XString>& parts, const wchar_t* separator) const
{
    int index = Find(separator, 0);
    if (index == -1) {
        parts.addElement(*this);
        return;
    }
    if (index != 0) {
        parts.addElement(SubString(0, index));
    }
    // Only one character is skipped: the separator is expected to be a single character.
    SubString(index + 1, Length()).Split(parts, separator);
}

XString XString::Replace(const wchar_t* from, const wchar_t* to, bool all) const
{
    int fromBytes = wcslen(from) * sizeof(wchar_t);
    int toBytes = wcslen(to) * sizeof(wchar_t);
    XString result(*this);
    int position = 0;
    for (;;) {
        position = result.FindData(from, fromBytes, position);
        if (position < 0) {
            break;
        }
        if (fromBytes != toBytes) {
            const char* chars = (const char*)result.m_data;
            int tailStart = position + fromBytes;
            XString replaced(chars, position, to, toBytes, chars + tailStart, result.ByteSize() - tailStart);
            result.Assign(replaced);
        } else {
            // Same size: overwrite in place (after making the block private).
            if (result.GetData()->m_refCount != 1) {
                result.Assign(result.m_data, result.ByteSize());
            }
            memcpy((char*)result.m_data + position, to, toBytes);
        }
        if (!all) {
            break;
        }
        position = position + toBytes;
    }
    return result;
}

wchar_t XString::Char(int index) const
{
    if (index >= 0 && index < Length()) {
        return m_data[index];
    }
    return 0;
}

XString& XString::SetChar(int index, wchar_t character)
{
    if (index >= 0 && index < Length()) {
        if (GetData()->m_refCount != 1) {
            Assign(m_data, ByteSize());
        }
        m_data[index] = character;
    }
    return *this;
}

XString& XString::InsertData(int startByte, const void* bytes, int byteCount)
{
    if (startByte >= 0) {
        const char* oldChars = (const char*)m_data;
        int oldSize = ((const Data*)oldChars - 1)->m_byteSize;
        if (startByte <= oldSize) {
            Init(oldChars, startByte, bytes, byteCount, oldChars + startByte, oldSize - startByte);
            ((Data*)oldChars - 1)->Release();
        }
    }
    return *this;
}

XString XString::SubData(int startByte, int byteCount) const
{
    int size = ByteSize();
    if (startByte < 0) {
        startByte = startByte + size;
        if (startByte < 0) {
            startByte = 0;
        }
    }
    if (startByte >= size) {
        return *pEmpty;
    }
    if (byteCount < 0) {
        byteCount = byteCount + (size - startByte);
        if (byteCount < 0) {
            return *pEmpty;
        }
    }
    if (byteCount + startByte > size) {
        byteCount = size - startByte;
    }
    if (startByte == 0 && byteCount == size) {
        return *this;
    }
    return XString((const char*)m_data + startByte, byteCount);
}

XString& XString::Append(const XString& other)
{
    return Append(other.m_data, other.ByteSize());
}

XString& XString::Append(const void* bytes, int byteCount)
{
    if (byteCount > 0) {
        int oldSize = ByteSize();
        int newSize = oldSize + byteCount;
        SetSize(newSize, newSize / 2, true);
        memcpy((char*)m_data + oldSize, bytes, byteCount);
    }
    return *this;
}

XString XString::Lower() const
{
    XString result(m_data, ByteSize());
    for (wchar_t* character = result.m_data; *character != 0; character++) {
        *character = CStdUtil_Cocoa::ToWLower(*character);
    }
    return result;
}

XString XString::Upper() const
{
    XString result(m_data, ByteSize());
    for (wchar_t* character = result.m_data; *character != 0; character++) {
        *character = CStdUtil_Cocoa::ToWUpper(*character);
    }
    return result;
}

int XString::CmpData(const void* first, int firstBytes, const void* second, int secondBytes, int maxBytes)
{
    int compared = secondBytes;
    if (firstBytes < secondBytes) {
        compared = firstBytes;
    }
    if (compared > maxBytes) {
        compared = maxBytes;
    }
    int result = memcmp(first, second, compared);
    if (result == 0 && compared != maxBytes) {
        // Equal prefix: the shorter one comes first.
        if (firstBytes < secondBytes) {
            return -1;
        }
        if (firstBytes > secondBytes) {
            return 1;
        }
        return 0;
    }
    return result;
}

int XString::Cmp(const XString& other, int maxLength) const
{
    return CmpData(m_data, ByteSize(), other.m_data, other.ByteSize(), maxLength * sizeof(wchar_t));
}

XString XString::ToString(int value)
{
    return XString(value);
}

bool XString::ParseDecSimple(const wchar_t*& cursor, int& value)
{
    const wchar_t* start = cursor;
    int result = 0;
    while ((unsigned int)(*cursor - L'0') < 10) {
        char digit = (char)(*cursor) - '0';
        cursor++;
        result = result * 10 + digit;
    }
    value = result;
    return cursor > start;
}

int XString::ParseNum(const wchar_t*& cursor, double& value)
{
    int sign = 1;
    if (*cursor == L'-') {
        cursor++;
        sign = -1;
    }
    const wchar_t* start = cursor;

    int integerPart = 0;
    ParseDecSimple(cursor, integerPart);

    if (*cursor == L'.') {
        double result = integerPart;
        cursor++;
        double scale = 0.1;
        while ((unsigned int)(*cursor - L'0') < 10) {
            char digit = (char)(*cursor) - '0';
            cursor++;
            result = result + digit * scale;
            scale = scale * 0.1;
        }
        if (cursor == start) {
            value = 0;
            return 0;
        }
        if ((*cursor | 0x20) == L'e') {
            cursor++;
            bool negative = false;
            if (*cursor == L'-') {
                cursor++;
                negative = true;
            } else if (*cursor == L'+') {
                cursor++;
            }
            int exponent;
            ParseDecSimple(cursor, exponent);
            double factor = 1.0;
            for (int step = 0; step < exponent; step++) {
                factor = factor * 10.0;
            }
            if (negative) {
                result = result / factor;
            } else {
                result = result * factor;
            }
        }
        value = result * sign;
        return 1;
    }

    if (cursor == start) {
        value = 0;
        return 0;
    }
    integerPart = integerPart * sign;
    value = integerPart;
    return 2;
}

float XString::ToFloat() const
{
    double value;
    const wchar_t* cursor = m_data;
    if (ParseNum(cursor, value) == 0) {
        return 0.0f;
    }
    return (float)value;
}

int XString::ToInt() const
{
    const wchar_t* cursor = m_data;
    if (*cursor == L'-') {
        cursor++;
    }
    int result = 0;
    while ((unsigned int)(*cursor - L'0') < 10) {
        char digit = (char)(*cursor) - '0';
        cursor++;
        result = result * 10 + digit;
    }
    if (m_data[0] == L'-') {
        result = -result;
    }
    return result;
}

bool XString::ToBool() const
{
    return ToInt() != 0;
}

int XString::HashData(const void* bytes, int byteCount)
{
    // djb2 (hash * 33 + c), unrolled by eight.
    const char* cursor = (const char*)bytes;
    int hash = 5381;
    for (; byteCount >= 8; byteCount -= 8) {
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
        hash = hash * 33 + *cursor++;
    }
    switch (byteCount) {
    case 7:
        hash = hash * 33 + *cursor++;
    case 6:
        hash = hash * 33 + *cursor++;
    case 5:
        hash = hash * 33 + *cursor++;
    case 4:
        hash = hash * 33 + *cursor++;
    case 3:
        hash = hash * 33 + *cursor++;
    case 2:
        hash = hash * 33 + *cursor++;
    case 1:
        hash = hash * 33 + *cursor++;
        break;
    default:
        break;
    }
    return hash;
}

XString::AnsiString::AnsiString(const wchar_t* text)
{
    int length = wcslen(text);
    m_text = (char*)malloc(length + 1);
    wcstombs(m_text, text, wcslen(text) + 1);
    m_text[length] = 0;
}
