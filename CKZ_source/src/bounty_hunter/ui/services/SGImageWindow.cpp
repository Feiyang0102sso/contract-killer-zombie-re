// b081j: original 15a330..15b358 complete SG window owner; keeps the original coordinates and scissor behavior.
#include "SGImageWindow.h"
#include "SGImageWindowCommand.h"
#include "SGButton.h"
#include "SScreenAdaptingConstant.h"
#include "WindowApp.h"
#include "App.h"
#include "BaseDialog.h"
#include "ICGraphics2d.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"

using namespace com::glu::platform::graphics;

static SScreenAdaptingConstant STILL_FIRE_DRAG_DELTA(10);

SGImageWindow::SGImageWindow() : Window(false), m_repeatCount(-1),
    m_closeWhenFinished(false), m_blinkCommand(0), m_imageRect(), m_blinking(false), m_frameBounds(), m_translateCoords(true)
{
}

SGImageWindow::~SGImageWindow() {}

void SGImageWindow::SetBlink(bool enabled)
{
    m_blinking = enabled;
    if (!enabled) {
        SetOpacity(1.0f);
    }
}

void SGImageWindow::SetBlinkCmd(unsigned int command) { m_blinkCommand = command; }

void SGImageWindow::SetTranslateCoords(bool enabled)
{
    m_translateCoords = enabled;
    UpdateSize();
}

void SGImageWindow::UpdateSize()
{
    if (!m_presenter.IsViable()) {
        return;
    }
    m_presenter.SetPosition(0, 0);
    m_presenter.Bounds(m_frameBounds);
    int width;
    int height;
    if (m_translateCoords) {
        m_presenter.SetPosition(-m_frameBounds.m_x, -m_frameBounds.m_y);
        width = m_frameBounds.m_width;
        height = m_frameBounds.m_height;
    } else {
        width = m_frameBounds.m_x + m_frameBounds.m_width;
        height = m_frameBounds.m_y + m_frameBounds.m_height;
    }
    // The original keeps the result across two layout external calls and does not re-read bounds after the callback.
    SetDesiredWidth(width);
    SetDesiredHeight(height);
    m_imageX = 0;
    m_imageY = 0;
    m_imageWidth = width;
    m_imageHeight = height;
}

void SGImageWindow::SetArchetypeCharacter(int archetype, int character)
{
    App::LoadSGArchetype(archetype, character);
    m_presenter.SetArchetypeCharacter(archetype, character);
}

void SGImageWindow::SetAnimation(int animation, unsigned char repeat)
{
    m_presenter.SetAnimation(animation, repeat);
    UpdateSize();
}

void SGImageWindow::SetRepeatCount(int count, bool closeWhenFinished)
{
    m_repeatCount = count;
    m_closeWhenFinished = closeWhenFinished;
}

void SGImageWindow::SetScale(int percent)
{
    m_presenter.setScale((unsigned short)percent);
    UpdateSize();
}

void SGImageWindow::Init() {} // Original 15a594 is a really empty method.

void SGImageWindow::Update()
{
    Window::Update();
    if (m_presenter.IsViable()) {
        m_presenter.Update(WindowApp::m_instance->m_updateFrameTime);
        if (m_presenter.HasFinished()) {
            if (m_repeatCount > 0) {
                m_presenter.Reset();
                --m_repeatCount;
            } else if (m_repeatCount == 0 && m_closeWhenFinished) {
                ReverseEasy(0x200);
                --m_repeatCount;
            }
        }
    }
    if (m_blinking) {
        unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
        float distance = (int)(time % 600 - 300);
        SetOpacity(MathLib::Fabs(distance) / 360.0f + 0.2f);
    }
}

void SGImageWindow::Paint(ICGraphics2d& graphics)
{
    if (m_presenter.IsViable()) {
        m_presenter.Draw();
    }
    Window::Paint(graphics);
}

void SGImageWindow::OnCommand(Event& event)
{
    if ((unsigned int)event.m_id == m_blinkCommand) {
        m_blinking = event.m_param1 != 0;
        if (event.m_param1 == 0) {
            SetOpacity(1.0f);
        }
    }
}

SGImageWindowCommand::SGImageWindowCommand() : m_command(0), m_commandParameter1(0),
    m_commandParameter2(0), m_commandParameter3(0), m_expandDragArea(true),
    m_pressed(false), m_hitPaddingX(0), m_hitPaddingY(0) {}

void SGImageWindowCommand::OnPointerEvent(Event& event)
{
    bool hit = false;
    if ((unsigned int)(event.m_type - Event::POINTER_DRAGGED) <= 1) {
        float factor = 1.0f;
        if (m_expandDragArea) {
            factor = 1.5f;
        }
        float extension = factor - 1.0f;
        float width = m_width + m_hitPaddingX;
        float height = m_height + m_hitPaddingY;
        float x = event.m_param1;
        float y = event.m_param2;
        if (x >= -(width * extension) && x < width * factor &&
            y >= -(height * extension) && y < height * factor) {
            float delta = m_pressGlobalX - m_parent->LocalToGlobalX(m_x);
            if (MathLib::Fabs(delta) < STILL_FIRE_DRAG_DELTA()) {
                // The original also uses LocalToGlobalX for the Y anchor; must not be changed to Y.
                delta = m_pressGlobalY - m_parent->LocalToGlobalX(m_y);
                hit = MathLib::Fabs(delta) < STILL_FIRE_DRAG_DELTA();
            }
        }
    }
    if (event.m_type == Event::POINTER_PRESSED) {
        m_pressed = true;
        m_pressGlobalX = m_parent->LocalToGlobalX(m_x);
        m_pressGlobalY = m_parent->LocalToGlobalX(m_y);
        event.Clear();
    } else if (event.m_type == Event::POINTER_RELEASED) {
        m_pressed = false;
        if (hit != 1) {
            return;
        }
        OnFire();
        if (m_command != 0) {
            WindowApp::HandleTunnelCommand(m_command, m_commandParameter1,
                                         m_commandParameter2, m_commandParameter3);
            SetBlink(false);
        }
        event.Clear();
    } else if (event.m_type == Event::POINTER_DRAGGED) {
        m_pressed = hit;
        event.Clear();
    }
}

void SGImageWindowCommand::Update()
{
    SGImageWindow::Update();
    if (m_pressed && m_command != 0) {
        SetOpacity(0.7f);
    } else if (!m_blinking) {
        SetOpacity(1.0f);
    }
}

bool SGImageWindowCommand::IsLocalPos(int x, int y) const
{
    return x >= -m_hitPaddingX && x < m_width + m_hitPaddingX &&
           y >= -m_hitPaddingY && y < m_height + m_hitPaddingY;
}

SGImageWindowRotatable::SGImageWindowRotatable() : m_angle(0)
{
    m_translateCoords = false;
}

void SGImageWindowRotatable::Paint(ICGraphics2d& graphics)
{
    graphics.PushTransform();
    int x = 0;
    int y = 0;
    LocalToGlobal(x, y);
    graphics.LoadIdentity();
    graphics.Translate(x, y);
    graphics.Rotate((float)m_angle);
    ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(
        x - m_width, y - m_height, m_width * 2, m_height * 2);
    SGImageWindow::Paint(graphics);
    graphics.PopTransform();
}

bool SGHudImageWindow::CheckCollision(int x, int y, float factor)
{
    float extension = factor - 1.0f;
    int extraX = (m_width * extension) * 0.5f;
    int extraY = (m_height * extension) * 0.5f;
    if (x <= m_x - extraX || x >= m_x + m_width + extraX ||
        y <= m_y - extraY || y >= m_y + m_height + extraY) {
        return false;
    }
    return IsVisible();
}

SGClippedImageWindow::SGClippedImageWindow() : m_clipX(1.0f), m_clipY(1.0f)
{
    m_translateCoords = false;
    UpdateSize();
}

void SGClippedImageWindow::SetClipping(float x, float y)
{
    m_clipX = x;
    m_clipY = y;
}

void SGClippedImageWindow::Paint(ICGraphics2d& graphics)
{
    if (!m_presenter.IsViable()) {
        return;
    }
    // b117-vb: the saved scissor is a value-initialized CRectangle<short> (original zeroes it as
    // two words in an 8-byte aligned stack slot, hence the bic sp,#7 realignment).
    com::glu::platform::components::CRectangle<short> oldScissor = com::glu::platform::components::CRectangle<short>();
    unsigned char enabled = 0;
    ICGraphics::GetInstance()->GetRasterizerState()->GetScissorState(enabled, oldScissor.m_x, oldScissor.m_y,
        oldScissor.m_width, oldScissor.m_height);
    ICRasterizerState* raster = ICGraphics::GetInstance()->GetRasterizerState();
    raster->SetScissor(oldScissor.m_x + m_frameBounds.m_x, m_frameBounds.m_y + oldScissor.m_y,
        (short)(m_frameBounds.m_width * m_clipX), (short)(m_frameBounds.m_height * m_clipY));
    m_presenter.Draw();
    // When the old scissor was not enabled, the original does not restore/disable the clipping just set.
    if (enabled) {
        ICGraphics::GetInstance()->GetRasterizerState()->SetScissor(oldScissor.m_x, oldScissor.m_y,
            oldScissor.m_width, oldScissor.m_height);
    }
}

SGToggleButton::SGToggleButton(int archetype, int character, int normalAnimation,
                             int selectedAnimation) : m_selected(false), m_allowTurnOff(true)
{
    SGImageWindow* image = new SGImageWindow;
    image->SetArchetypeCharacter(archetype, character);
    image->SetAnimation(normalAnimation, 1);
    m_normalImage = image;
    AddToFront(image);
    image = new SGImageWindow;
    image->SetArchetypeCharacter(archetype, character);
    image->SetAnimation(selectedAnimation, 1);
    m_selectedImage = image;
    AddToFront(image);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    UpdateToggle();
}

void SGToggleButton::UpdateToggle()
{
    if (m_selected) {
        m_normalImage->ClearFlags(1);
    } else {
        m_normalImage->SetFlags(1);
    }
    if (m_selected) {
        m_selectedImage->SetFlags(1);
    } else {
        m_selectedImage->ClearFlags(1);
    }
}

void SGToggleButton::OnFire()
{
    if (!m_allowTurnOff && m_selected) {
        return;
    }
    m_selected = !m_selected;
    UpdateToggle();
}

SGButton::SGButton(int archetype, int character, int normalAnimation, int pressedAnimation)
    : m_flagf4(false)
{
    WindowTransparent* container = new WindowTransparent(false);
    container->SetWidthByContent(0, 0);
    container->SetHeightByContent(0, 0);
    SGImageWindow* image = new SGImageWindow;
    image->SetArchetypeCharacter(archetype, character);
    image->SetAnimation(normalAnimation, 1);
    m_normalImage = image;
    container->AddToFront(image);
    image = new SGImageWindow;
    image->SetArchetypeCharacter(archetype, character);
    image->SetAnimation(pressedAnimation, 1);
    m_pressedImage = image;
    container->AddToFront(image);
    m_textWindow = new TextWindow;
    m_textWindow->SetAlign(0x24);
    m_textWindow->SetHeightByContent(0, 0);
    m_textWindow->SetWidthByContent(0, 0);
    container->AddToFront(m_textWindow);
    SetHeightByContent(0, 0);
    SetWidthByContent(0, 0);
    AddToFront(container);
}

TextWindow* SGButton::SetText(const XString& text, eGameFont font)
{
    TextWindow* window = m_textWindow;
    window->SetText(text, CFontMgr::GetInstance()->GetFont(font));
    return m_textWindow;
}

void SGButton::Update()
{
    if (m_pressed != m_blinking) {
        m_normalImage->ClearFlags(1);
        m_pressedImage->SetFlags(1);
    } else {
        if (m_flagf4) {
            m_normalImage->ClearFlags(1);
        } else {
            m_normalImage->SetFlags(1);
        }
        if (m_flagf4) {
            m_pressedImage->SetFlags(1);
        } else {
            m_pressedImage->ClearFlags(1);
        }
    }
    if (m_flagf4) {
        ClearFlags(0x2000);
    } else {
        SetFlags(0x2000);
    }
    BlinkingButton::Update();
}

void BlinkingButton::Update()
{
    if (m_blinking) {
        unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
        float distance = (int)(time % 600 - 300);
        SetOpacity(MathLib::Fabs(distance) / 360.0f + 0.2f);
    } else {
        SetOpacity(1.0f);
    }
}
