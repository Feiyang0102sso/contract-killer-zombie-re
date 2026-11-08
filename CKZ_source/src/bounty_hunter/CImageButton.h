// CImageButton: original 11ccf4..11d008 complete owner, real CBH_Button inheritance, total 0x100.
#ifndef CIMAGEBUTTON_H
#define CIMAGEBUTTON_H
#include "CBH_Button.h"
#include "Vector_ss.h"
class TextWindow;
class ImageWindow;

class CImageButton : public CBH_Button {
public:
    CImageButton();
    virtual ~CImageButton();
    virtual void Update();
    void AddImage(const char* name, int x = 0, int y = 0);
    void AddImage(com::glu::platform::graphics::ICRenderSurface* surface, int x = 0, int y = 0);
    void SetImage(com::glu::platform::graphics::ICRenderSurface* surface, int x = 0, int y = 0);
    void SetText(XString text, int alignment, com::glu::platform::graphics::CFont* font,
                 int leftSpacing, int percentHeight);

    int m_unresolvedE4;             // 0xe4, not written by the original constructor
    Vector<XString> m_imageNames;   // 0xe8, holds the resource name
    TextWindow* m_textWindow;      // 0xf8, owned by the Window tree
    ImageWindow* m_imageWindow;    // 0xfc
};
#endif
