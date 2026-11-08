// CSystemFont_Cocoa.h - system font drawn with UIKit (created by b011x, owner of
// cocoa/src/CSystemFont_Cocoa_mm.mm).
#ifndef CSYSTEMFONT_COCOA_H
#define CSYSTEMFONT_COCOA_H

#include "CSystemFont.h"

#ifdef __OBJC__
@class NSString;
@class UIFont;
#else
typedef struct objc_object NSString;
typedef struct objc_object UIFont;
#endif

class CSystemFont_Cocoa : public com::glu::platform::graphics::CSystemFont {
public:
    CSystemFont_Cocoa();
    virtual ~CSystemFont_Cocoa();

    virtual void Destroy();
    virtual int GetFontHeight() const;
    virtual int GetMinLineSpacing() const;
    virtual int GetMinCharSpacing() const;
    virtual int GetWidth(wchar_t character) const;
    virtual int MeasureTextWidth(const wchar_t* text, int length, int maxWidth, int* fitCount);
    virtual unsigned char InitializeSurfaceWithText(com::glu::platform::graphics::ICRenderSurface& surface,
                                                    const wchar_t* text, int length, int maxWidth, int maxHeight);
    virtual unsigned char FinalizeInitialization(const unsigned char* data, unsigned int size);

    /** Draw `text` white on transparent into a width x height PNG and initialize `surface` with it. */
    unsigned char CreatePngWithText(const NSString* text, int width, int height,
                                    com::glu::platform::graphics::ICRenderSurface& surface);

    UIFont* m_font;     // 0x4c autoreleased UIFont (not retained)
};

#endif
