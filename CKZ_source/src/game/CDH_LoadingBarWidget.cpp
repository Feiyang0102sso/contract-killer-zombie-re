// CDH_LoadingBarWidget.cpp - loading bar at the bottom of the loading screen.
#include "CDH_LoadingBarWidget.h"
#include "CFontMgr.h"
#include "CFont.h"
#include "CUtility.h"
#include "App.h"
#include "DGHelper.h"
#include "ICRenderSurface.h"
#include "ICGraphics2d.h"

using namespace com::glu::platform;

/** Font of the text. */
static const int LOADING_FONT = 0x1a;
/** Blink period of the text (ms shown, then ms hidden). */
static const int BLINK_TIME = 400;
/** Bottom margin of the bar (pixels). */
static const int BAR_BOTTOM_MARGIN = 5;

CDH_LoadingBarWidget::CDH_LoadingBarWidget()
    : Window(false)
{
    m_progress = 0;
    m_font = CFontMgr::GetInstance()->GetFont(LOADING_FONT);
    setState(STATE_LOADING);
    m_stripes[0].Create("SUR_LOADING_STRIPE_00");
    m_stripes[1].Create("SUR_LOADING_STRIPE_01");
}

void CDH_LoadingBarWidget::setState(eState state)
{
    m_state = state;
    switch (state) {
    case STATE_LOADING:
        m_blinking = false;
        CUtility::GetString(m_text, "IDS_LOADING");
        break;
    case STATE_CONNECTING:
        m_blinking = true;
        m_text = "Connecting";
        break;
    case STATE_TAP:
        m_blinking = true;
        m_blinkTime = 0;
        CUtility::GetString(m_text, "IDS_TAP_TO_CONTINUE");
        break;
    }
    m_textLength = m_text.m_length;
}

void CDH_LoadingBarWidget::Update()
{
    Window::Update();
    if (m_blinking) {
        m_blinkTime += WindowApp::m_instance->m_updateFrameTime;
        if (m_blinkTime > BLINK_TIME) {
            m_blinkTime = -BLINK_TIME;
        }
    }
}

void CDH_LoadingBarWidget::Paint(graphics::ICGraphics2d& graphics)
{
    int screenWidth = ((App*)WindowApp::m_instance)->m_helper->m_screenWidth;
    int screenHeight = ((App*)WindowApp::m_instance)->m_helper->m_screenHeight;
    graphics.PushTransform();
    unsigned int width = 0;
    unsigned int height = 0;
    graphics::ICRenderSurface* background = m_stripes[0].GetSurface();
    graphics::ICRenderSurface* progress = m_stripes[1].GetSurface();
    background->GetWidthAndHeight(width, height);

    components::CRectangle<short> source = components::CRectangle<short>();
    source.m_x = 0;
    source.m_y = 0;
    source.m_width = width;
    source.m_height = height;
    int x = (screenWidth - width) / 2;
    int bottom = screenHeight - BAR_BOTTOM_MARGIN;
    graphics.Translate((float)x, (float)(bottom - height));
    graphics.Draw(*background, &source, &WindowApp::m_instance->Color(), (graphics::ICGraphics2d::Flip)0);
    source.m_width = (short)width * m_progress / 100;
    graphics.Draw(*progress, &source, &WindowApp::m_instance->Color(), (graphics::ICGraphics2d::Flip)0);
    graphics.PopTransform();

    m_font->GetFontHeight();
    if (!m_blinking || m_blinkTime < 0) {
        const wchar_t* text = m_text.m_string;
        int textWidth = m_font->MeasureTextWidth(text, -1, -1, NULL);
        m_font->PaintText(text, m_textLength, x + (width - textWidth) / 2,
                          bottom - (m_font->GetFontHeight() + height) / 2, -1, -1, &WindowApp::m_instance->Color());
    }
}
