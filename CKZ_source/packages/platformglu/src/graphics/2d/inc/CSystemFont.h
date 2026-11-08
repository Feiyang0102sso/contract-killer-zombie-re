// CSystemFont.h - font drawn by the platform: the text is rendered into a surface by a 2d text
// renderer (ICRenderText2d) and the surface is drawn. sizeof == 0x4c.
// (taken over by b034x from b011x's PROVISIONAL; owner graphics/2d/src/CSystemFont.cpp)
#ifndef CSYSTEMFONT_H
#define CSYSTEMFONT_H

#include <stddef.h>
#include "CFont.h"
#include "CStrWChar.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICRenderSurface;
class ICRenderText2d;

class CSystemFont : public CFont {
public:
    /** Header of the font description (4 bytes, copied as is). */
    struct FontHeader {
        unsigned int m_version;     // 0x00
    };

    /** The last text drawn and its renderer, reused while the text does not change. */
    class Text2dCache {
    public:
        inline Text2dCache()
        {
            m_font = NULL;
            m_renderText = NULL;
            m_text = "";
            m_length = 0;
            m_width = 0;
            m_height = 0;
        }

        inline ~Text2dCache()
        {
            Destroy();
        }

        /** Re-render when the text or the box changed. */
        void Update(const wchar_t* text, int length, int width, int height);
        void Destroy();

        CFont* m_font;                      // 0x00 font the text is rendered with
        components::CStrWChar m_text;       // 0x04
        int m_length;                       // 0x14
        int m_width;                        // 0x18
        int m_height;                       // 0x1c
        ICRenderText2d* m_renderText;       // 0x20
    };

    CSystemFont();
    virtual ~CSystemFont();

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router);
    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual unsigned int GetType() const;
    virtual void PaintText(const wchar_t* text, int length, int x, int y, int width, int height,
                           const components::Color_RGBA* color);
    /** Render `text` into `surface` (at most maxWidth x maxHeight, -1 = unlimited). */
    virtual unsigned char InitializeSurfaceWithText(ICRenderSurface& surface, const wchar_t* text, int length,
                                                    int maxWidth, int maxHeight) = 0;
    /** Platform part of Initialize(): parse the description and create the native font. */
    virtual unsigned char FinalizeInitialization(const unsigned char* data, unsigned int size) = 0;
    /** Read the font description (name, size, antialiasing). */
    virtual unsigned char Parse(const unsigned char* data, unsigned int size);

    unsigned char* m_data;              // 0x08 description data while initializing
    FontHeader* m_header;               // 0x0c
    unsigned char m_antialias;          // 0x10
    int m_pointSize;                    // 0x14
    components::CStrWChar m_fontName;   // 0x18 empty: the system font
    Text2dCache m_textCache;            // 0x28
};

} } } }

#endif
