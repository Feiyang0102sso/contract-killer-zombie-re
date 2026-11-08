// ICRenderText2d.h - 2d text model: a text in a font, ready to be drawn by ICGraphics2d.
// (b002 declared the factory, defined in cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp; base class,
// InitParam / Description and the slots added by b034x from the CRenderText2d vtable)
#ifndef ICRENDERTEXT2D_H
#define ICRENDERTEXT2D_H

#include "ICGraphicsResource.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
}

namespace graphics {

class ICFont;

class ICRenderText2d : public ICGraphicsResource {
public:
    /** Key of the text resources (registry element type). */  // added by b034x
    enum { ResourceType = 0x2df4b4c6 };  // added by b034x

    /** InitParam keys. */  // added by b034x
    enum InitParamKey {  // added by b034x
        InitParam_End = 0,  // added by b034x
        InitParam_Font = 1,             // ICFont*  // added by b034x
        InitParam_Text = 2,             // pointer to the text (not copied)  // added by b034x
        InitParam_TextType = 3,         // MimeType of InitParam_Text  // added by b034x
        InitParam_Stream = 4,           // CInputStream* with the text  // added by b034x
        InitParam_StreamType = 5,       // MimeType of the stream content  // added by b034x
        InitParam_FormatArguments = 6,  // void** printf arguments: the text is a format  // added by b034x
        InitParam_Offset = 7,           // first character  // added by b034x
        InitParam_Length = 8            // characters (< 0x10000)  // added by b034x
    };  // added by b034x

    /** Text encodings. */  // added by b034x
    enum MimeType {  // added by b034x
        MimeType_Char = 0x00025e62,     // char string  // added by b034x
        MimeType_WChar = 0x00215e62,    // wchar_t string  // added by b034x
        MimeType_StrChar = 0x64365e6e,  // components::CStrChar  // added by b034x
        MimeType_StrWChar = 0x43735eb4, // components::CStrWChar  // added by b034x
        MimeType_Utf8 = 0xf686aadc      // UTF-8 bytes  // added by b034x
    };  // added by b034x

    /** One (key, value) Initialize() setting; a list ends with key 0. */  // added by b034x
    struct InitParam {  // added by b034x
        unsigned int m_key;     // 0x00  // added by b034x
        unsigned int m_value;   // 0x04  // added by b034x
    };  // added by b034x

    /** What GetDescription() reports. */  // added by b034x
    struct Description {  // added by b034x
        ICGraphicsAbstract::Abstraction m_abstraction;  // 0x00  // added by b034x
        ICFont* m_font;                 // 0x04  // added by b034x
        const void* m_text;             // 0x08  // added by b034x
        unsigned int m_textType;        // 0x0c  // added by b034x
        unsigned short m_length;        // 0x10  // added by b034x
    };  // added by b034x

    /** Platform factory (CGraphicsAbstractionManager_Cocoa.cpp). */
    static ICRenderText2d* CreateInstance();

    ICRenderText2d() : ICGraphicsResource(ResourceType) {}  // added by b034x

    virtual ~ICRenderText2d() {}

    virtual unsigned char Load(components::CInputStream& stream, unsigned int type) = 0;  // added by b034x
    virtual unsigned char Initialize(const InitParam* params) = 0;  // added by b034x
    virtual void Destroy() = 0;  // added by b034x
    /** Fill `description`; returns 0 when the text is not initialized for an abstraction. */  // added by b034x
    virtual unsigned char GetDescription(Description& description) = 0;  // added by b034x
};

} } } }

#endif
