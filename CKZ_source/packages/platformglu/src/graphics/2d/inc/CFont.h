// CFont.h - common base of the fonts (bitmap font, system font): the font interface after Load().
// (taken over by b034x from b011x's PROVISIONAL; owner graphics/2d/src/CFont.cpp)
// Slot order from the CBitmapFont / CSystemFont vtables.
#ifndef CFONT_H
#define CFONT_H

#include <wchar.h>
#include "ICFont.h"

namespace com { namespace glu { namespace platform {

namespace components {
struct Color_RGBA;
}

namespace graphics {

class CFont : public ICFont {
public:
    /** Runtime class id shared by every font. */
    enum { ClassId = 0x02d70994 };

    /** Character classes used by the line breaker (CTextParser). */
    enum TokenType {
        TokenType_Normal = 0,
        TokenType_End = 1,          // NUL: end of the text
        TokenType_Space = 2,        // a line may break before it
        TokenType_Tab = 3,
        TokenType_NewLine = 4,
        TokenType_Break = 5         // '-' / '@': a line may break after it
    };

    CFont()
    {
        m_classId = ClassId;
    }

    virtual ~CFont() {}

    virtual unsigned char Initialize(const InitParam* params) = 0;
    virtual void Destroy() = 0;
    virtual unsigned int GetType() const = 0;
    /** TokenType of `character`. */
    virtual int GetTokenType(wchar_t character) const;
    virtual int GetFontHeight() const = 0;
    virtual int GetMinLineSpacing() const = 0;
    virtual int GetMinCharSpacing() const = 0;
    virtual int GetWidth(wchar_t character) const = 0;
    /** Width of `text` (length -1: NUL terminated) cut at `maxWidth`; *fitCount gets the characters that fit. */
    virtual int MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount) = 0;
    virtual void PaintText(const wchar_t* text, int length, int x, int y, int width, int height,
                           const components::Color_RGBA* color) = 0;

    unsigned int m_classId;         // 0x04
};

} } } }

#endif
