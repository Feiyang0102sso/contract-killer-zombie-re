// LanguageDialog.cpp - first start dialog that lets the player choose the game language.
#include "LanguageDialog.h"
#include "App.h"
#include "AppSettings.h"
#include "CFontMgr.h"
#include "ICMediaPlayer.h"
#include "YesNoDialog.h"
#include "Interpolation.h"

using namespace com::glu::platform::components;

/** Fonts of the language list (CFontMgr table indices). */
static const int FONT_ITEM = 5;
static const int FONT_ITEM_SELECTED = 6;
/** Easing: fly in from the bottom edge of the parent. */
static const float SHOW_TIME = 1.05f;
static const float HIDE_TIME = 0.7f;
/** Easing commands. */
static const int COMMAND_SHOWN = 0x100;
static const int COMMAND_LANGUAGE_CHOSEN = 0x200;

SelectLanguageDialog::SelectLanguageDialog()
    : SimpleDialog(DIALOG_TYPE_1)
{
    App::CreateGameRunningFile();
    ICMediaPlayer::GetInstance()->SetSoundEnabled(false);
    m_itemFont = CFontMgr::GetInstance()->GetFont(FONT_ITEM);
    m_selectedItemFont = CFontMgr::GetInstance()->GetFont(FONT_ITEM_SELECTED);
    m_selectedLanguage = -1;
}

SimpleDialog::ItemFonts SelectLanguageDialog::ItemFontsByIndex(int index) const
{
    return ItemFonts(m_itemFont, m_selectedItemFont);
}

void SelectLanguageDialog::InitCenter()
{
    SetCenter((int)(m_parent->m_width * 0.5f), (int)(m_parent->m_height * 0.5f));
}

void SelectLanguageDialog::AdjustByDefRect()
{
    int width;
    int height;
    if (m_parent != NULL) {
        width = m_parent->m_width;
        height = m_parent->m_height;
    } else {
        width = WindowApp::m_instance->m_screenWidth;
        height = WindowApp::m_instance->m_screenHeight;
    }
    int margin = m_margins[0];
    AdjustByRect(margin * 2, 0, width - margin * 4, height);
}

void SelectLanguageDialog::Init()
{
    SimpleDialog::Init();
    Close();
}

void SelectLanguageDialog::Shutdown()
{
    if (m_selectedLanguage >= 0) {
        ((App*)WindowApp::m_instance)->m_settings->SetLocale(m_selectedLanguage, true);
    }
    Window* parent = m_parent;
    if ((parent->m_flags & FLAG_CLOSING) == 0) {
        parent->AddModal(new GameStartSoundDialog());
    }
    SimpleDialog::Shutdown();
}

void SelectLanguageDialog::OnStartupEasy()
{
    EasyParams params(this, EasyLib::OUT_BOUNCE, EasyLib::IN_BACK);
    params.SetFullTimes(SHOW_TIME, HIDE_TIME);
    params.SetMoving(m_x, m_parent->m_height, m_x, m_y);
    params.SetOpacity(0.0f, 1.0f);
    StartEasy(params, COMMAND_SHOWN);
}

void SelectLanguageDialog::ClickItem(int index, bool byPointer)
{
    m_selectedLanguage = index;
    ReverseEasy(COMMAND_LANGUAGE_CHOSEN);
}
