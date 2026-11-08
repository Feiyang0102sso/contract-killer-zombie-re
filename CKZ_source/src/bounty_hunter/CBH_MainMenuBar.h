// b074: menu bar owner ABI and the original aggregate descriptor.
#ifndef CBH_MAINMENUBAR_H
#define CBH_MAINMENUBAR_H
#include "WindowBase.h"
class CBH_MenuButton;
struct MenuButtonDesc {
    const char* m_image;
    const char* m_pressedImage;
    int m_command;
    unsigned int m_blinkCommand;
    const char* m_caption;
};
class CBH_MainMenuBar : public Window {
public:
    CBH_MainMenuBar(int x, int y, int width, int height);
    virtual ~CBH_MainMenuBar() { ReleaseResources(); }
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    virtual void OnPointerEvent(Event& event);
    virtual void OnCommand(Event& event);
    virtual void ReleaseResources(); // bc
    void SetButtons(const MenuButtonDesc* buttons, int count);
    Vector<CBH_MenuButton*> m_buttons; // ac
    com::glu::platform::components::CRectangle<short> m_backgroundRect; // bc
    ImageRes m_background; // c4
    ImageRes m_separator;  // cc
};
#endif
