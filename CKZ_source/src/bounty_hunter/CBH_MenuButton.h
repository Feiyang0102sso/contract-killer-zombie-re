// b074: MenuButton's complete ABI, including the four added virtual slots c0..cc.
#ifndef CBH_MENUBUTTON_H
#define CBH_MENUBUTTON_H
#include "CBH_Button.h"

class CBH_MenuButton : public CBH_Button {
public:
    CBH_MenuButton();
    virtual ~CBH_MenuButton() { ReleaseResources(); }
    virtual void Init();
    virtual void OnCommand(Event& event);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void ReleaseResources();
    virtual void SetCorner(int x, int y);
    virtual int Left() { return m_x + m_padding; }
    virtual int Top() { return m_y + m_padding; }
    void setImage(const char* name);
    void UpdateSize();
    void setPressedImage(const char* name);
    void SetCaption(const char* name);
    void SetDesiredSizes();
    void SetBlinkingCmd(unsigned int command);

    com::glu::platform::graphics::ICRenderSurface* m_image;        // e4
    com::glu::platform::graphics::ICRenderSurface* m_pressedImage; // e8
    com::glu::platform::graphics::ICRenderSurface* m_blinkImage;   // ec
    XString m_caption;                                          // f0
    com::glu::platform::graphics::CFont* m_font;                  // f4
    int m_padding;                                              // f8
    bool m_blinking;                                            // fc
    unsigned int m_blinkCommand;                                 // 100
    int m_unresolved104;                                        // 104
};
#endif
