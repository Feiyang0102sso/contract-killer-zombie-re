// bvsprintf.cpp - bounded printf-style formatting into char / wchar_t buffers.
// Derived from the classic Linux kernel lib/vsprintf.c (vsprintf + number), extended with an
// output budget: every character written consumes one unit of the remaining buffer size.
#include "bvsprintf.h"

#include <stddef.h>

// ---- format flags ----
#define ZEROPAD     1       // pad with zero
#define SIGN        2       // unsigned/signed long
#define PLUS        4       // show plus
#define SPACE       8       // space if plus
#define LEFT        16      // left justified
#define SPECIAL     32      // 0x / 0 prefix
#define LARGE       64      // use 'ABCDEF' instead of 'abcdef'
#define NO_NUMBER   128     // never set by the parser; numeric output is skipped when present

// Store one character while the buffer has room; otherwise leave the innermost loop or switch.
#define PUT_OR_BREAK(character) if (bufferSize-- > 0) { *str++ = (character); } else break

// Store one character while the buffer has room (pf_number keeps counting when full).
#define PUT_IF_ROOM(character) if (remaining-- > 0) { *str++ = (character); }

// ---- constant data ----
static const char NULL_STR[8] = "<NULL>";
static const wchar_t NULL_STR_w[] = { 'N', 'U', 'L', 'L' };     // not terminated (as shipped)
static const wchar_t wdigits[] = L"0123456789abcdefghijklmnopqrstuvwxyz";
static const wchar_t wdigitsL[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define is_digit(c) ((c) >= '0' && (c) <= '9')

/** Parse a decimal number at *text and advance past it. */
static inline int skip_atoi(const char** text)
{
    int value = 0;
    while (is_digit(**text)) {
        value = value * 10 + *((*text)++) - '0';
    }
    return value;
}

/** Wide version of skip_atoi(const char**). */
static inline int skip_atoi(const wchar_t** text)
{
    int value = 0;
    while (is_digit(**text)) {
        value = value * 10 + *((*text)++) - '0';
    }
    return value;
}

static char* pf_number(char* str, long num, int base, int size, int precision, int type, int& remaining);
static wchar_t* pf_number(wchar_t* str, long num, int base, int size, int precision, int type, int& remaining);

int bvsprintf_s(char* buffer, int bufferSize, const char* format, va_list args)
{
    int length;
    int index;
    int base;
    int flags;          // flags to pf_number()
    int fieldWidth;     // width of output field
    int precision;      // min. # of digits for integers; max number of chars for from string
    int qualifier;      // 'h', 'l', or 'L' for integer fields
    char* str;
    const char* text;

    if (buffer == NULL || format == NULL) {
        return 0;
    }

    // keep one slot for the terminator
    bufferSize--;

    for (str = buffer; *format != '\0'; ++format) {
        if (*format != '%') {
            PUT_OR_BREAK(*format);
            continue;
        }

        // process flags
        flags = 0;
repeat:
        ++format;   // this also skips first '%'
        switch (*format) {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // get field width
        fieldWidth = -1;
        if (is_digit(*format)) {
            fieldWidth = skip_atoi(&format);
        } else if (*format == '*') {
            ++format;
            // it's the next argument
            fieldWidth = va_arg(args, int);
            if (fieldWidth < 0) {
                fieldWidth = -fieldWidth;
                flags |= LEFT;
            }
        }

        // get the precision
        precision = -1;
        if (*format == '.') {
            ++format;
            if (*format >= '0') {
                if (*format <= '9') {
                    precision = skip_atoi(&format);
                }
            } else if (*format == '*') {
                ++format;
                // it's the next argument
                precision = va_arg(args, int);
            }
            if (precision < 0) {
                precision = 0;
            }
        }

        // get the conversion qualifier
        qualifier = -1;
        if (*format == 'h' || *format == 'l' || *format == 'L') {
            qualifier = *format;
            ++format;
        }

        // default base
        base = 10;

        switch (*format) {
        case 'c':
            if (!(flags & LEFT)) {
                while (--fieldWidth > 0) {
                    PUT_OR_BREAK(' ');
                }
            }
            PUT_OR_BREAK((unsigned char)va_arg(args, int));
            while (--fieldWidth > 0) {
                PUT_OR_BREAK(' ');
            }
            continue;

        case 'U':
        case 's':
            text = va_arg(args, const char*);
            if (text == NULL) {
                text = NULL_STR;
            }

            // strnlen(text, precision)
            for (length = 0; length != precision && text[length] != '\0'; length++) {
            }

            if (!(flags & LEFT)) {
                while (length < fieldWidth--) {
                    PUT_OR_BREAK(' ');
                }
            }
            for (index = 0; index < length; ++index) {
                PUT_OR_BREAK(*text++);
            }
            while (length < fieldWidth--) {
                PUT_OR_BREAK(' ');
            }
            continue;

        case 'p':
            if (fieldWidth == -1) {
                fieldWidth = 2 * sizeof(void*);
                flags |= ZEROPAD;
            }
            str = pf_number(str, (long)va_arg(args, void*), 16, fieldWidth, precision, flags, bufferSize);
            continue;

        case 'n': {
            int* written = va_arg(args, int*);
            *written = str - buffer;
            continue;
        }

        // integer number formats - set up the flags and "break"
        case 'o':
            base = 8;
            break;

        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;

        default:
            // unknown conversion (or "%%"): echo it
            if (*format != '%') {
                PUT_OR_BREAK('%');
            }
            if (*format != '\0') {
                PUT_OR_BREAK(*format);
            } else {
                --format;
            }
            continue;
        }

        long number = va_arg(args, long);
        if (!(flags & NO_NUMBER)) {
            str = pf_number(str, number, base, fieldWidth, precision, flags, bufferSize);
        }
    }

    *str = '\0';
    return str - buffer;
}

/** Format one integer; writes at most `remaining` characters but always consumes the full width. */
static char* pf_number(char* str, long num, int base, int size, int precision, int type, int& remaining)
{
    char padding;
    char sign;
    char digitsReversed[66];
    const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    int count;

    if (type & LARGE) {
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }
    if (type & LEFT) {
        type &= ~ZEROPAD;
    }
    if (base < 2 || base > 36) {
        return 0;
    }

    // ternary kept: the if/else form selects the other way round in the original code
    padding = (type & ZEROPAD) ? '0' : ' ';

    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }

    count = 0;
    if (num == 0) {
        digitsReversed[count++] = '0';
    } else {
        while (num != 0) {
            digitsReversed[count++] = digits[((unsigned long)num) % (unsigned)base];
            num = ((unsigned long)num) / (unsigned)base;
        }
    }
    if (count > precision) {
        precision = count;
    }
    size -= precision;

    if (!(type & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            PUT_IF_ROOM(' ');
        }
    }
    if (sign) {
        PUT_IF_ROOM(sign);
    }
    if (type & SPECIAL) {
        if (base == 8) {
            PUT_IF_ROOM('0');
        } else if (base == 16) {
            PUT_IF_ROOM('0');
            PUT_IF_ROOM(digits[33]);
        }
    }
    if (!(type & LEFT)) {
        while (size-- > 0) {
            PUT_IF_ROOM(padding);
        }
    }
    while (count < precision--) {
        PUT_IF_ROOM('0');
    }
    while (count-- > 0) {
        PUT_IF_ROOM(digitsReversed[count]);
    }
    while (size-- > 0) {
        PUT_IF_ROOM(' ');
    }
    return str;
}

int bwsprintf_s(wchar_t* buffer, int bufferSize, const wchar_t* format, va_list args)
{
    int length;
    int index;
    int base;
    int flags;          // flags to pf_number()
    int fieldWidth;     // width of output field
    int precision;      // min. # of digits for integers; max number of chars for from string
    int qualifier;      // 'h', 'l', or 'L' for integer fields
    wchar_t* str;
    const wchar_t* text;
    const wchar_t* scan;

    if (buffer == NULL || format == NULL) {
        return 0;
    }

    // keep one slot for the terminator
    bufferSize--;

    for (str = buffer; *format != L'\0'; ++format) {
        if (*format != L'%') {
            PUT_OR_BREAK(*format);
            continue;
        }

        // process flags
        flags = 0;
repeat:
        ++format;   // this also skips first '%'
        switch (*format) {
        case L'-':
            flags |= LEFT;
            goto repeat;
        case L'+':
            flags |= PLUS;
            goto repeat;
        case L' ':
            flags |= SPACE;
            goto repeat;
        case L'#':
            flags |= SPECIAL;
            goto repeat;
        case L'0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // get field width
        fieldWidth = -1;
        if (is_digit(*format)) {
            fieldWidth = skip_atoi(&format);
        } else if (*format == L'*') {
            ++format;
            // it's the next argument
            fieldWidth = va_arg(args, int);
            if (fieldWidth < 0) {
                fieldWidth = -fieldWidth;
                flags |= LEFT;
            }
        }

        // get the precision
        precision = -1;
        if (*format == L'.') {
            ++format;
            if (*format >= L'0') {
                if (*format <= L'9') {
                    precision = skip_atoi(&format);
                }
            } else if (*format == L'*') {
                ++format;
                // it's the next argument
                precision = va_arg(args, int);
            }
            if (precision < 0) {
                precision = 0;
            }
        }

        // get the conversion qualifier
        qualifier = -1;
        if (*format == L'h' || *format == L'l' || *format == L'L') {
            qualifier = *format;
            ++format;
        }

        // default base
        base = 10;

        switch (*format) {
        case L'c':
            if (!(flags & LEFT)) {
                while (--fieldWidth > 0) {
                    PUT_OR_BREAK(L' ');
                }
            }
            PUT_OR_BREAK((unsigned char)va_arg(args, int));
            while (--fieldWidth > 0) {
                PUT_OR_BREAK(L' ');
            }
            continue;

        case L'U':
        case L's':
            text = va_arg(args, const wchar_t*);
            if (text == NULL) {
                text = NULL_STR_w;
            }

            // wcsnlen(text, precision)
            for (scan = text; precision-- && *scan != L'\0'; ++scan) {
            }
            length = scan - text;

            if (!(flags & LEFT)) {
                while (length < fieldWidth--) {
                    PUT_OR_BREAK(L' ');
                }
            }
            for (index = 0; index < length; ++index) {
                PUT_OR_BREAK(*text++);
            }
            while (length < fieldWidth--) {
                PUT_OR_BREAK(L' ');
            }
            continue;

        case L'p':
            if (fieldWidth == -1) {
                fieldWidth = 2 * sizeof(void*);
                flags |= ZEROPAD;
            }
            str = pf_number(str, (long)va_arg(args, void*), 16, fieldWidth, precision, flags, bufferSize);
            continue;

        case L'n': {
            int* written = va_arg(args, int*);
            *written = str - buffer;
            continue;
        }

        // integer number formats - set up the flags and "break"
        case L'o':
            base = 8;
            break;

        case L'X':
            flags |= LARGE;
        case L'x':
            base = 16;
            break;

        case L'd':
        case L'i':
            flags |= SIGN;
        case L'u':
            break;

        default:
            // unknown conversion (or "%%"): echo it
            if (*format != L'%') {
                PUT_OR_BREAK(L'%');
            }
            if (*format != L'\0') {
                PUT_OR_BREAK(*format);
            } else {
                --format;
            }
            continue;
        }

        long number = va_arg(args, long);
        if (!(flags & NO_NUMBER)) {
            str = pf_number(str, number, base, fieldWidth, precision, flags, bufferSize);
        }
    }

    *str = L'\0';
    return str - buffer;
}

/** Wide version of pf_number(char*, ...). */
static wchar_t* pf_number(wchar_t* str, long num, int base, int size, int precision, int type, int& remaining)
{
    wchar_t padding;
    wchar_t sign;
    wchar_t digitsReversed[66];
    const wchar_t* digits = wdigits;
    int count;

    if (type & LARGE) {
        digits = wdigitsL;
    }
    if (type & LEFT) {
        type &= ~ZEROPAD;
    }
    if (base < 2 || base > 36) {
        return 0;
    }

    if (type & ZEROPAD) {
        padding = L'0';
    } else {
        padding = L' ';
    }

    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = L'-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = L'+';
            size--;
        } else if (type & SPACE) {
            sign = L' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }

    count = 0;
    if (num == 0) {
        digitsReversed[count++] = L'0';
    } else {
        while (num != 0) {
            digitsReversed[count++] = digits[((unsigned long)num) % (unsigned)base];
            num = ((unsigned long)num) / (unsigned)base;
        }
    }
    if (count > precision) {
        precision = count;
    }
    size -= precision;

    if (!(type & (ZEROPAD + LEFT))) {
        while (size-- > 0) {
            PUT_IF_ROOM(L' ');
        }
    }
    if (sign) {
        PUT_IF_ROOM(sign);
    }
    if (type & SPECIAL) {
        if (base == 8) {
            PUT_IF_ROOM(L'0');
        } else if (base == 16) {
            PUT_IF_ROOM(L'0');
            PUT_IF_ROOM(digits[33]);
        }
    }
    if (!(type & LEFT)) {
        while (size-- > 0) {
            PUT_IF_ROOM(padding);
        }
    }
    while (count < precision--) {
        PUT_IF_ROOM(L'0');
    }
    while (count-- > 0) {
        PUT_IF_ROOM(digitsReversed[count]);
    }
    while (size-- > 0) {
        PUT_IF_ROOM(L' ');
    }
    return str;
}

int bvsprintf(char* buffer, const char* format, va_list args)
{
    return bvsprintf_s(buffer, 0x7FFFFFFF, format, args);
}

int bwsprintf(wchar_t* buffer, const wchar_t* format, va_list args)
{
    return bwsprintf_s(buffer, 0x7FFFFFFF, format, args);
}
