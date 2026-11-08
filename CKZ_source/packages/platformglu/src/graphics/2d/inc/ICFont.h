// PROVISIONAL (created by b011x): owner packages/platformglu/src/cocoa/src/CGraphicsAbstractionManager_Cocoa.cpp not written yet
// ICFont.h - font interface (bitmap or system font). Only what the resource system needs.
#ifndef ICFONT_H
#define ICFONT_H

namespace com { namespace glu { namespace platform {

namespace components {
class CInputStream;
class CIdToObjectRouter;
}

namespace graphics {

class ICFont {
public:
    /** Font implementations (values from CResourceFont::CreateInternal). */
    enum Type {
        Type_Bitmap = 0,
        Type_System = 1
    };

    /** One (key, value) Initialize() setting; a list ends with key 0. */
    struct InitParam {
        unsigned int m_key;     // 0x00
        unsigned int m_value;   // 0x04
    };

    /** Platform factory. */
    static ICFont* CreateInstance(Type type);

    virtual ~ICFont() {}

    /** Load the font description of mime type `type` from `stream`. */
    virtual unsigned char Load(components::CInputStream& stream, unsigned int type,
                               components::CIdToObjectRouter* router) = 0;

    /** InitParam keys (values from CBitmapFont / CSystemFont Load and Initialize). */  // added by b034x
    enum InitParamKey {  // added by b034x
        InitParam_End = 0,              // terminates the list  // added by b034x
        InitParam_Image = 1,            // ICRenderSurface* with the glyphs  // added by b034x
        InitParam_ImageType = 2,        // type reported by the router for InitParam_Image  // added by b034x
        InitParam_ImageStream = 3,      // CInputStream* with the glyph image (not supported)  // added by b034x
        InitParam_ImageStreamType = 4,  // added by b034x
        InitParam_Data = 5,             // object holding the font description (CBinary for system fonts)  // added by b034x
        InitParam_DataType = 6,         // added by b034x
        InitParam_DataStream = 7,       // CInputStream* with the font description  // added by b034x
        InitParam_DataStreamType = 8    // added by b034x
    };  // added by b034x

    /** Mime type of a font description. */  // added by b034x
    enum { MimeType_FontData = 0xf4e02223 };  // added by b034x
};

} } } }

#endif
