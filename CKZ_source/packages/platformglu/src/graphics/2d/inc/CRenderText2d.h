// CRenderText2d.h - the 2d text model of the platform device: the text (referenced or decoded from a
// stream) and, for system fonts, the surface it is rendered into.
// (taken over by b034x from b002's PROVISIONAL; owner graphics/2d/src/CRenderText2d.cpp) sizeof == 0x1c.
#ifndef CRENDERTEXT2D_H
#define CRENDERTEXT2D_H

#include <stddef.h>
#include "ICRenderText2d.h"
#include "CFont.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICRenderSurface;

class CRenderText2d : public ICRenderText2d {
public:
    /** TextModel::m_flags bits. */
    enum Flag {
        Flag_BitmapFont = 0x0001,       // drawn glyph by glyph
        Flag_SystemFont = 0x0002,       // rendered into m_surface
        Flag_Software = 0x0020,         // belongs to the software abstraction
        Flag_Hardware = 0x0040,         // belongs to the device abstraction
        Flag_TextReferenced = 0x0800,   // m_text points into the caller's text (not owned)
        Flag_Initialized = 0x8000
    };

    /** State of the text (value initialized by the constructor). */
    struct TextModel {
        unsigned short m_flags;         // 0x08 Flag bits
        unsigned short m_length;        // 0x0a characters
        unsigned int m_textType;        // 0x0c MimeType of m_text
        const void* m_text;             // 0x10
        CFont* m_font;                  // 0x14
        ICRenderSurface* m_surface;     // 0x18 system fonts: the rendered text
    };

    CRenderText2d();
    virtual ~CRenderText2d();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual unsigned char Load(components::CInputStream& stream, unsigned int type);
    virtual unsigned char Initialize(const InitParam* params);
    virtual void Destroy();
    virtual unsigned char GetDescription(Description& description);

    /** Clip [offset, offset + span) to the length of `text` (of mime type `type`). */
    static void ClipOffsetAndSpan(const void* text, unsigned int type, unsigned int& offset, unsigned short& span);
    /**
     * New wchar_t copy of `span` characters from `offset` of the text in `stream` (encoding `type`),
     * formatted with `formatArguments` when not NULL; *text is NULL when nothing was read.
     */
    static void CreateModelWStr(wchar_t** text, components::CInputStream& stream, unsigned int type,
                                unsigned int offset, unsigned int span, void** formatArguments);
    /** Render the text into m_surface with the system font. */
    unsigned char AssocSystemFont();

    TextModel m_model;                  // 0x08
};

} } } }

#endif
