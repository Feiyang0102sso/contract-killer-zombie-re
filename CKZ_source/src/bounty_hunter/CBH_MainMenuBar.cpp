// b074: original 0x1179f4..0x117d34, including Window-owned child lifetime.
#include "CBH_MainMenuBar.h"
#include "CBH_MenuButton.h"
#include "ServiceWindows.h"

void CBH_MainMenuBar::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    DrawTileImageRect(graphics, m_background.GetSurface(), m_backgroundRect.m_x,
        m_backgroundRect.m_y, m_backgroundRect.m_width, m_backgroundRect.m_height);
    DrawTileImageHoriz(graphics, m_separator.GetSurface(), 0, 0, m_width);
}

void CBH_MainMenuBar::OnPointerEvent(Event& event)
{
    if (event.m_type != Event::POINTER_PRESSED) {
        return;
    }
    int left = m_backgroundRect.m_x;
    int x = event.m_param1;
    if (x <= left || x >= left + m_backgroundRect.m_width) {
        return;
    }
    int top = m_backgroundRect.m_y;
    int y = event.m_param2;
    if (y <= top || y >= top + m_backgroundRect.m_height) {
        return;
    }
    event.Clear();
}

CBH_MainMenuBar::CBH_MainMenuBar(int, int, int, int)
    : Window(false), m_buttons(), m_backgroundRect(), m_background(), m_separator()
{
    // All four original parameters are unused. The rectangle is value-initialized.
}

void CBH_MainMenuBar::ReleaseResources()
{
    if (m_buttons.m_items != NULL) {
        free(m_buttons.m_items);
        m_buttons.m_items = NULL;
    }
    m_buttons.m_size = 0;
    m_buttons.m_capacity = 0;
}

void CBH_MainMenuBar::OnCommand(Event&)
{
    // Original 0x117b3c is a return.
}

void CBH_MainMenuBar::SetButtons(const MenuButtonDesc* buttons, int count)
{
    Window* container = new Window(false);
    container->SetPercentHeight(90, 0, 0);
    container->SetLayoutType(LAYOUT_SIMPLE);
    container->SetAlign(0x10);
    for (int index = 0; index < count; ++index) {
        CBH_MenuButton* button = new CBH_MenuButton();
        button->setImage(buttons[index].m_image);
        button->SetAlign(0x24);
        button->setPressedImage(buttons[index].m_pressedImage);
        button->m_command = buttons[index].m_command;
        button->m_commandParameter1 = 0;
        button->m_commandParameter2 = 0;
        button->m_commandParameter3 = 0;
        button->SetCaption(buttons[index].m_caption);
        button->SetBlinkingCmd(buttons[index].m_blinkCommand);
        container->AddToFront(createTD(button, index, 0, 3));
        m_buttons.addElement(button);
    }
    AddToFront(container);
}
