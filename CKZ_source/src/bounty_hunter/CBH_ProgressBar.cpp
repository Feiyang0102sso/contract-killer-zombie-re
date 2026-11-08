// b074: complete original progress bar. Integer ratio and unsigned image dimensions are intentional.
#include "CBH_ProgressBar.h"
#include "CBH_SurfaceManager.h"
#include "SScreenAdaptingConstant.h"
#include "App.h"
#include "CFontMgr.h"
#include "DGHelper.h"

// Original 4dd640 has all four entries; Tab uses color=3, the last three must stay NULL, do not restore only the first string.
static const char* COLOR_IMAGES[] = { "", NULL, NULL, NULL };
static SScreenAdaptingConstant TEXT_Y_SHIFT(2);

void CBH_ProgressBar::Init()
{
    // Original return-only method.
}

void CBH_ProgressBar::Update()
{
    Window::Update();
}

void CBH_ProgressBar::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    unsigned int left = 0;
    int top = 0;
    if (m_desc != NULL) {
        left = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_desc) >> 1;
        unsigned int descHeight = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_desc);
        unsigned int leftHeight = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_left);
        if (descHeight > leftHeight) {
            top = (descHeight >> 1) - (leftHeight >> 1);
        }
    }
    DrawImage(graphics, m_left, left, top, 0, 0);
    int fillX = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_right) + left;
    com::glu::platform::graphics::ICRenderSurface* image = m_fill;
    int fillWidth = GetFillWidth();
    DrawTileImageHoriz(graphics, image, fillX, top, fillWidth);
    if ((m_barFlags & 4) == 0) {
        // Original 118c0a caches percent across the two width queries and the first draw; the delta tail still rereads it.
        float percent = m_percent;
        float maximum = percent + m_delta;
        float minimum = percent;
        if (m_delta < 0.0f) {
            minimum = maximum;
        }
        int widthForMaximum = GetFillWidth();
        // ARM LT includes unordered: preserve that path rather than std::max.
        if (!(minimum >= 0.0f)) {
            minimum = 0.0f;
        }
        int widthForMinimum = GetFillWidth();
        image = m_color;
        float colorWidth = minimum * (float)widthForMinimum / 100.0f;
        int colorHeight = ImageHeight(image);
        int firstWidth = (int)colorWidth;
        DrawTileImageRect(graphics, image, fillX, top, firstWidth, colorHeight);
        if (percent > maximum) {
            maximum = percent;
        }
        float secondWidth = maximum * (float)widthForMaximum / 100.0f;
        if (m_secondColorName != NULL && m_delta != 0.0f) {
            int end = (int)secondWidth;
            image = m_secondColor;
            int height = ImageHeight(image);
            DrawTileImageRect(graphics, image, fillX + firstWidth, top, end - firstWidth, height);
        }
    }
    image = m_right;
    fillWidth = GetFillWidth();
    DrawImage(graphics, image, fillX + fillWidth, top, 0, 0);
    if (m_showText) {
        com::glu::platform::graphics::CFont* font = CFontMgr::GetInstance()->GetFont(8);
        int x = m_textX;
        int y = m_textY;
        const wchar_t* text = m_text.m_data;
        int shift = TEXT_Y_SHIFT();
        DrawFontText(graphics, font, text, x, y + shift, m_fontAlign);
    }
    if (m_desc != NULL) {
        DrawImage(graphics, m_desc, 0, 0, 0, 0);
    }
    Window::Paint(graphics);
}

int CBH_ProgressBar::GetFillWidth()
{
    int width = m_width;
    unsigned int desc = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_desc);
    unsigned int left = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_left);
    unsigned int right = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceWidth(m_right);
    return width - desc - left - right;
}

CBH_ProgressBar::CBH_ProgressBar(int color, int width, int secondColor) : CBH_Button(), m_text()
{
    m_delta = 0.0f;
    m_percent = 0.0f;
    m_leftName = "";
    m_rightName = "";
    m_fillName = "";
    m_desc = NULL;
    m_displayFormat = 0;
    m_showText = true;
    m_colorName = COLOR_IMAGES[color];
    m_secondColorName = NULL;
    if (secondColor >= 0) {
        m_secondColorName = COLOR_IMAGES[secondColor];
    }
    CreateImpl(width);
}

void CBH_ProgressBar::CreateImpl(int width)
{
    m_left = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_leftName, true, false);
    m_right = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_rightName, true, false);
    m_fill = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_fillName, true, false);
    m_color = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_colorName, true, false);
    if (m_secondColorName != NULL) {
        m_secondColor = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(m_secondColorName, true, false);
    }
    SetLayoutType(LAYOUT_NONE);
    int height = ((App*)WindowApp::m_instance)->m_surfaceManager->GetSurfaceHeight(m_fill);
    SetDesiredWidth(width);
    SetDesiredHeight(height);
    SetSize(width, height);
    m_descName = NULL;
    m_maximum = -1;
    m_value = -1;
    SetTextAlign(TEXT_CENTER);
    m_barFlags = 3;
}

CBH_ProgressBar::~CBH_ProgressBar()
{
    ReleaseResources();
}

void CBH_ProgressBar::SetTextAlign(TextAlign align)
{
    m_textAlign = align;
    if (align == TEXT_LEFT) {
        m_textX = ImageWidth(m_desc);
        m_textY = m_height / 2;
        m_fontAlign = 1;
    } else if (align == TEXT_OUTSIDE) {
        int fillWidth = GetFillWidth();
        m_textX = fillWidth + ImageWidth(m_right);
        int offset = 10;
        if (App::IsHD() != 0) {
            offset = 20;
        }
        m_textX += offset;
        m_textY = m_height / 2;
        m_fontAlign = 0x22;
    } else if (align == TEXT_CENTER) {
        m_textX = m_width / 2;
        m_textY = m_height / 2;
        m_fontAlign = 0x24;
    }
}

void CBH_ProgressBar::SetDescImage(const char* name)
{
    if (m_desc != NULL && m_descName != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_descName);
    }
    m_desc = ((App*)WindowApp::m_instance)->m_surfaceManager->CreateSurface(name, true, false);
    m_descName = name;
    m_height = ImageHeight(m_desc);
    SetSize(m_width, m_height);
    SetDesiredHeight(m_height);
    SetTextAlign(m_textAlign);
}

void CBH_ProgressBar::ReleaseResources()
{
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_leftName);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_rightName);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_fillName);
    ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_colorName);
    if (m_descName != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_descName);
    }
    if (m_secondColorName != NULL) {
        ((App*)WindowApp::m_instance)->m_surfaceManager->ReleaseSurface(m_secondColorName);
    }
}

void CBH_ProgressBar::SetCurrValue(int value)
{
    if (m_value != value) {
        m_value = value;
        UpdateBarParams();
    }
}

void CBH_ProgressBar::UpdateBarParams()
{
    if ((m_barFlags & 3) == 3) {
        m_text.Assign(*XString::pEmpty);
        if (m_displayFormat == 0) {
            m_text.Append(XString::ToString(m_value));
            if (m_maximum >= 1) {
                m_text.Append(L"/", wcslen(L"/") * sizeof(wchar_t));
                m_text.Append(XString::ToString(m_maximum));
            }
        } else if (m_displayFormat == 1) {
            m_text.Append(DGHelper::ClockStringFromSeconds(m_value));
        }
    } else if ((m_barFlags & 1) != 0) {
        m_text.Assign(*XString::pEmpty);
        m_text.Assign(XString::ToString(m_value));
    } else if ((m_barFlags & 8) != 0) {
        m_text.Assign(*XString::pEmpty);
    }
    if ((m_barFlags & 0x20) == 0) {
        if (m_maximum < 1) {
            m_percent = 0.0f;
        } else {
            m_percent = (m_value * 100) / m_maximum;
        }
    }
}
