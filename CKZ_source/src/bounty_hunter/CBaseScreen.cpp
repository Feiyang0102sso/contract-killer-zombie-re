// b074: complete screen owner and its true local BlackWindow, 0x119b04..0x11a0ec.
#include "CBaseScreen.h"
#include "SScreenAdaptingConstant.h"
#include "SGImageWindow.h"
#include "SGImageWindowCommand.h"
#include "CGearSettingsButton.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "CSettingsWindow.h"
#include "CDrawUtil.h"
#include "App.h"
#include "CDH_SoundManager.h"

// Original merged-data names were lost; values/ownership from 4ff2c0 and 4ff2d0.
static SScreenAdaptingConstant titleBarHeight(40);
static SScreenAdaptingConstant titleDecorationHeight(50);

CBaseScreen::CBaseScreen(bool showBack, bool showTitleBar, bool showSettings) : Window(false)
{
    class BlackWindow : public Window {
    public:
        BlackWindow() : Window(false) {}
        virtual ~BlackWindow() {}
        virtual void Paint(com::glu::platform::graphics::ICGraphics2d&)
        {
            int width = m_width;
            int height = m_height;
            unsigned int color = (int)(WindowApp::m_instance->Opacity() * 255.0f) << 24;
            CDrawUtil::Fill(0, 0, width, height, color, (com::glu::platform::graphics::ICGraphics2d::Primitive)3);
        }
    };
    m_showTitleBar = showTitleBar;
    m_back = NULL;
    m_titleHeight = titleBarHeight();
    if (m_showTitleBar) {
        SGImageWindow* header = new SGImageWindow();
        header->SetArchetypeCharacter(1, 0);
        header->SetAnimation(30, 1);
        header->SetLayoutType(LAYOUT_NONE);
        header->SetCorner((WindowApp::m_instance->m_screenWidth - header->m_imageWidth) / 2, 0);
        AddToFront(header);
        SGImageWindow* decoration = new SGImageWindow();
        decoration->SetArchetypeCharacter(1, 0);
        decoration->SetAnimation(51, 1);
        decoration->SetLayoutType(LAYOUT_SIMPLE);
        decoration->SetAlign(0x14);
        decoration->SetOutsetSpacing(0, 0, titleDecorationHeight(), 0);
        AddToFront(decoration);
        BlackWindow* fill = new BlackWindow();
        fill->SetLayoutType(LAYOUT_SIMPLE);
        fill->SetAlign(0x14);
        int width = WindowApp::m_instance->m_screenWidth;
        fill->SetSize(width, titleDecorationHeight());
        AddToFront(fill);
    }
    if (showBack == 1) {
        AddBackButton();
    }
    if (showSettings == 1) {
        AddSettingsButton();
    }
    m_title = new TextWindow(CFontMgr::GetInstance()->GetFont(29));
    m_title->SetLayoutType(LAYOUT_NONE);
    // Title starts right of the back button when there is one.
    if (showBack == 1) {
        m_title->SetCorner(m_back->m_x + m_back->m_width + 10, 0);
    } else {
        m_title->SetCorner(10, 0);
    }
    m_content = new Window(false);
    m_content->SetLayoutType(LAYOUT_NONE);
    // titleBarHeight() is an argument expression: the original loads the SetRect slot before calling it.
    m_content->SetRect(0, titleBarHeight(), m_width - m_x, m_height - m_titleHeight, false);
    AddToFront(m_content);
    AddToFront(m_title);
}

void CBaseScreen::AddBackButton()
{
    m_back = new SGImageWindowCommand();
    m_back->SetArchetypeCharacter(1, 0);
    m_back->SetAnimation(4, 1);
    m_back->SetAlign(9);
    m_back->m_command = 0x5f523cd9;
    m_back->m_commandParameter1 = 0;
    m_back->m_commandParameter2 = 0;
    m_back->m_commandParameter3 = 0;
    SGImageWindowCommand* button = m_back;
    int left = SScreenAdaptingConstant(7)();
    int bottom = SScreenAdaptingConstant(2)();
    button->SetOutsetSpacing(left, 0, 0, bottom);
    button = m_back;
    int paddingX = SScreenAdaptingConstant(20)();
    int paddingY = SScreenAdaptingConstant(20)();
    button->m_hitPaddingX = paddingX;
    button->m_hitPaddingY = paddingY;
    m_back->SetBlinkCmd(0x2dd14cf3);
    AddToFront(m_back);
}

void CBaseScreen::AddSettingsButton()
{
    CGearSettingsButton* button = new CGearSettingsButton();
    button->SetLayoutType(LAYOUT_SIMPLE);
    button->SetAlign(10);
    button->m_command = 0x8e576746;
    button->m_commandParameter1 = 0;
    button->m_commandParameter2 = 0;
    button->m_commandParameter3 = 0;
    int left = SScreenAdaptingConstant(19)();
    int top = SScreenAdaptingConstant(15)();
    button->SetOutsetSpacing(left, top, 0, 0);
    AddToFront(button);
}

void CBaseScreen::Paint(com::glu::platform::graphics::ICGraphics2d&)
{
    // Original return-only method.
}

CBaseScreen::~CBaseScreen()
{
    // Window's destructor owns the child hierarchy.
}

int CBaseScreen::GetTitleBarHeight()
{
    return m_titleHeight;
}

void CBaseScreen::OnCommand(Event& event)
{
    // b147: original places the back (0x5f523cd9) block first, so it is written first.
    if ((unsigned int)event.m_id == 0x5f523cd9) {
        ReverseEasy(0x200);
        event.Clear();
    } else if ((unsigned int)event.m_id == 0x8e576746) {
        CSettingsWindow* settings = new CSettingsWindow(false, false);
        settings->SetLayoutType(LAYOUT_SIMPLE);
        settings->SetAlign(0x24);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
        event.Clear();
    }
}

void CBaseScreen::SetTitleText(XString text)
{
    m_title->SetText(text);
    TextWindow* title = m_title;
    if (m_back != NULL) {
        int left = m_back->m_x;
        int width = m_back->m_width;
        int scale = App::Scale(); // Original keeps this query even though x does not use it.
        title->SetCorner(left + width + 10, scale * 5);
    } else {
        int left = App::Scale() * 5;
        title->SetCorner(left * 4, App::Scale() * 5);
    }
}

void CBaseScreen::Init()
{
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)94, false, false);
}

void CBaseScreen::Shutdown()
{
    ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)94, false, false);
}

void CBaseScreen::OnStartupEasy()
{
    EasyParams params(this, (EasyLib::EMethod)6);
    params.SetFullTime(0.5f);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, 0x100);
}
