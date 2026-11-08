// CBitmapFont.h - font drawn from a glyph sheet (a render surface) and a metrics table.
// (taken over by b034x from b002's PROVISIONAL; owner graphics/2d/src/CBitmapFont.cpp)
#ifndef CBITMAPFONT_H
#define CBITMAPFONT_H

#include "CFont.h"
#include "CSharedPtr.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICRenderSurface;

class CBitmapFont : public CFont {
public:
    /** Header of the metrics data (12 bytes, copied as is). */
    struct FontHeader {
        unsigned char m_format;         // 0x00 2: character codes stored as a JM UTF string
        unsigned char m_reserved1[4];   // 0x01
        signed char m_ascent;           // 0x05
        signed char m_charSpacing;      // 0x06 extra space between characters
        signed char m_lineSpacing;      // 0x07 extra space between lines
        short m_numChars;               // 0x08 entries of the character table
        short m_numControlChars;        // 0x0a entries of the control character table
    };

    /** Glyph of a printable character (12 bytes). */
    struct CharDesc {
        unsigned short m_code;          // 0x00 character code (the table is sorted by it)
        unsigned short m_x;             // 0x02 glyph position in the image
        unsigned short m_y;             // 0x04
        signed char m_width;            // 0x06 glyph size
        signed char m_height;           // 0x07
        signed char m_offsetX;          // 0x08 drawing offset
        signed char m_offsetY;          // 0x09
        signed char m_advance;          // 0x0a pen advance
        signed char m_reserved;         // 0x0b
    };

    /** Width of a control character (no glyph; 4 bytes). */
    struct ControlCharDesc {
        unsigned short m_code;          // 0x00
        signed char m_reserved;         // 0x02
        signed char m_advance;          // 0x03
    };

    CBitmapFont();
    virtual ~CBitmapFont();

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router);
    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual unsigned int GetType() const;
    virtual int GetFontHeight() const;
    virtual int GetMinLineSpacing() const;
    virtual int GetMinCharSpacing() const;
    virtual int GetWidth(wchar_t character) const;
    virtual int MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount);
    virtual void PaintText(const wchar_t* text, int length, int x, int y, int width, int height,
                           const components::Color_RGBA* color);

    /** Build the header and the character tables from the metrics data. */
    void ParseFontMetrics(const unsigned char* data, unsigned int size);
    /** Control character entry of `character`, NULL if none. */
    const ControlCharDesc* GetControlCharDesc(wchar_t character) const;
    /** Glyph of `character` (binary search), NULL if none. */
    const CharDesc* GetCharDesc(wchar_t character) const;

    unsigned char* m_data;                      // 0x08 metrics data while initializing
    ICRenderSurface* m_image;                   // 0x0c glyph sheet
    FontHeader* m_header;                       // 0x10
    CharDesc* m_charDescs;                      // 0x14
    ControlCharDesc* m_controlCharDescs;        // 0x18
    core::CSharedPtr<void*>* m_sharedImage;     // 0x1c keeps a routed glyph sheet alive
};

} } } }

#endif
