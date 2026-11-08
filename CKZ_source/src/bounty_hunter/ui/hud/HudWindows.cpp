// k: original 152d30..1532ec, complete 16 ranges; keeps NaN, negative countdown and the original clip-restore condition.
#include "HudWindows.h"
#include "WindowApp.h"
#include "CFontMgr.h"
#include "ICGraphics2d.h"
#include <math.h>
#include "MathLib.h"
#include "CRectangle.h"
using namespace com::glu::platform::graphics;

bool HudWindow::CheckCollision(int x, int y, float factor)
{
    int paddingX = (int)(m_width * (factor - 1.0f) * 0.5f);
    int paddingY = (int)(m_height * (factor - 1.0f) * 0.5f);
    if (x <= m_x - paddingX) {
        return false;
    }
    if (x >= m_x + m_width + paddingX) {
        return false;
    }
    if (y <= m_y - paddingY) {
        return false;
    }
    if (y >= m_y + m_height + paddingY) {
        return false;
    }
    return IsVisible(); // The original calls this only after passing the four-edge check.
}

void ProgressFillHudImageWindow::SetIsAnimating(bool enabled)
{
    if (m_isAnimating != enabled) {
        m_timeLeft = m_duration;
        m_isAnimating = enabled;
    }
}

void ProgressFillHudImageWindow::Update()
{
    if (m_timeLeft >= 1) {
        m_timeLeft -= WindowApp::m_instance->m_updateFrameTime;
    } else {
        m_timeLeft = 0;
    }
}

void ProgressFillHudImageWindow::Paint(ICGraphics2d& graphics)
{
    float ratio = (float)m_timeLeft / (float)m_duration;
    // The original NEON min keeps NaN; do not use C fminf, which would swallow NaN.
    if (ratio < 0.0f) {
        ratio = 0.0f;
    } else {
        ratio = (1.0f < ratio) ? 1.0f : ratio;
    }
    if (!m_isAnimating) {
        ratio = 1.0f - ratio;
    }
    BeginBlend(graphics, 0.7f * ratio + 0.3f, 1.0f, 1.0f, 1.0f, false, false);
    ImageWindow::Paint(graphics);
    EndBlend(graphics);
    if (!m_isAnimating) {
        return;
    }
    BeginBlend(graphics, 0.7f, m_blendGreen, m_blendBlue, m_blendAlpha, false, false);
    com::glu::platform::components::CRectangle<short> clip = {0, 0, 0, 0};
    unsigned char enabled = 0;
    ICRasterizerState* raster = ICGraphics::GetInstance()->GetRasterizerState();
    raster->GetScissorState(enabled, clip.m_x, clip.m_y, clip.m_width, clip.m_height);
    float progress = m_progress;
    // The original converts float directly to short; going through int then short adds an extra sxth and changes the 32-bit register/stack value passed in.
    raster->SetScissor(clip.m_x,
        (short)(clip.m_y + clip.m_height * (1.0f - progress)),
        clip.m_width, (short)(clip.m_height * progress));
    ImageWindow::Paint(graphics);
    if (enabled) {
        raster->SetScissor(clip.m_x, clip.m_y, clip.m_width, clip.m_height);
    }
    EndBlend(graphics);
}

ConsumableHudImageWindow::ConsumableHudImageWindow(const char* image) : m_count(-1)
{
    SetLayoutType(LAYOUT_SIMPLE);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    SGImageWindow* background = new SGImageWindow;
    background->SetArchetypeCharacter(0, 0);
    background->SetAnimation(21, 1);
    background->SetAlign(9);
    m_countText = new TextWindow;
    m_countText->SetAlign(36);
    background->AddToFront(m_countText);
    AddToFront(background);
    m_fill = new ProgressFillHudImageWindow;
    m_fill->SetImageById(image);
    m_fill->SetAlign(9);
    m_fill->SetOutsetSpacing((int)(background->m_imageHeight * 0.3f), 0, 0,
        (int)(background->m_imageWidth * 0.7f));
    AddToFront(m_fill);
}

void ConsumableHudImageWindow::SetCount(int count)
{
    if (m_count == count) {
        return;
    }
    m_count = count;
    TextWindow* target = m_countText;
    XString text(count);
    target->SetText(text, CFontMgr::GetInstance()->GetFont(25));
}

FlashingEffectWindow::FlashingEffectWindow() : m_elapsed(0), m_period(600)
{
    m_active = false;
    m_stopRequested = false;
    ClearFlags(1);
}

void FlashingEffectWindow::StartFlashing(int period)
{
    m_elapsed = 0;
    m_period = period;
    m_active = true;
    m_stopRequested = false;
    SetFlags(1);
}

void FlashingEffectWindow::Update()
{
    m_elapsed += WindowApp::m_instance->m_updateFrameTime;
    if (!m_active) {
        return;
    }
    int period = m_period;
    float distance = (float)(m_elapsed % period - period / 2);
    float opacity = MathLib::Fabs(distance) / ((period * 0.5f) * 1.2f) + 0.2f;
    if (m_stopRequested && MathLib::Fabs(opacity) < 0.3f) {
        m_active = false;
        m_stopRequested = false;
        ClearFlags(1);
    }
    SetOpacity(opacity); // Even if stopped this frame, the previously computed local opacity is still set.
}
