// YesNoDialog.cpp - question dialogs with "yes" / "no" items, the first start sound question
// (with a hidden cheat code input) and a message dialog with "OK".
#include "YesNoDialog.h"
#include "App.h"
#include "AppSettings.h"
#include "WindowApp.h"
#include "CFontMgr.h"
#include "CDH_SoundManager.h"
#include "ICMediaPlayer.h"
#include "CApplet.h"
#include "ICCore.h"
#include "Interpolation.h"

using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;

/** Fonts (CFontMgr table indices). */
static const int FONT_MESSAGE = 5;
static const int FONT_ITEM = 6;
/** Easing commands: shown / close the dialog. */
static const int COMMAND_SHOWN = 0x100;
static const int COMMAND_CLOSE = 0x200;
/** Easing of the dialog: grows from its center. */
static const float SHOW_TIME = 0.7f;
static const float SHOW_DELAY_X = 0.2f;
static const float SHOW_DELAY_Y = 0.1f;
static const int SHOW_FLAGS = 0x84000;
/** Cheat code: taps kept, length of the code, shortest prefix that is shown. */
static const int MAX_TAPS = 10;
static const int CHEAT_LENGTH = 9;
static const int CHEAT_MIN_PREFIX = 4;
static const int CHEAT_ALMOST_PREFIX = 7;
/** Taps of the cheat code. */
static const signed char TAP_LEFT = 1;
static const signed char TAP_RIGHT = 0;

const signed char GameStartSoundDialog::CheatsWindow::cheatTable[12] = {1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0};

YesNoDialog::YesNoDialog(const XString& text, int userValue1, int userValue2, int userValue3, CFont* font)
    : SimpleDialog(DIALOG_TYPE_1)
{
    // ?: keeps the call target loaded before the font is chosen (as in the original)
    m_textArea->SetFont(font != NULL ? font : CFontMgr::GetInstance()->GetFont(FONT_ITEM));
    SetText(text);
    m_userValue1 = userValue1;
    m_userValue2 = userValue2;
    m_userValue3 = userValue3;
    m_clickedItem = 0;
    m_field188 = false;
    AddItem(ITEM_YES, Window::ResString("IDS_YES"), ITEM_NORMAL);
    AddItem(ITEM_NO, Window::ResString("IDS_NO"), ITEM_NORMAL);
}

void YesNoDialog::InitCenter()
{
    SetCenter((int)(m_parent->m_width * 0.5f), (int)(m_parent->m_height * 0.5f));
}

void YesNoDialog::Init()
{
    SimpleDialog::Init();
    InitCenter();
    AdjustByDefRect();
    EasyParams params(this, EasyLib::OUT_BACK);
    params.SetFullTime(SHOW_TIME);
    params.SetDelayTimes(SHOW_DELAY_X, SHOW_DELAY_Y);
    params.m_clearFlagsOnStart = SHOW_FLAGS;
    params.m_setFlagsOnFinish = SHOW_FLAGS;
    params.m_opacityFrom = 0.0f;
    StartEasy(params, COMMAND_SHOWN);
}

void YesNoDialog::OnLayout()
{
    ClearFlags(FLAG_SHOW_AFTER_EASY);
    InitCenter();
    AdjustByDefRect();
    SimpleDialog::OnLayout();
}

void YesNoDialog::OnClosed()
{
}

void YesNoDialog::OnLostFocus()
{
    SimpleDialog::OnLostFocus();
}

void YesNoDialog::ClickItem(int id, bool byPointer)
{
    m_clickedItem = id;
    ReverseEasy(COMMAND_CLOSE);
}

GameStartSoundDialog::CheatsWindow::CheatsWindow()
    : WindowTransparent(false)
{
    ClearFlags(FLAG_POINTER_HIT_TEST);
    SetOutsetSpacing(10);
    SetLayoutType(LAYOUT_SIMPLE);
    SetAlign(ALIGN_LEFT | ALIGN_BOTTOM);
    SetWidthByContent(0, 0);
    SetHeightByContent(0, 0);
    m_cheatsEnabled = false;
    m_images[IMAGE_PARTIAL].Create(NULL);
    m_images[IMAGE_ALMOST].Create(NULL);
    m_images[IMAGE_ENABLED].Create(NULL);
    m_image = new ImageWindow(ImageWindow::FILL_NONE, ImageRes());
    m_image->SetCellPos(0, 0, 1, 1);
    AddToFront(m_image);
}

int GameStartSoundDialog::CheatsWindow::CheckCheatTable(int length)
{
    int start = m_taps.size() - length;
    if (start < 0) {
        return 0;
    }
    for (int index = 0; index < length; index++) {
        if (m_taps[start + index] != cheatTable[index]) {
            return 0;
        }
    }
    return 1;
}

void GameStartSoundDialog::CheatsWindow::OnPointerPressed(Event& event)
{
    if (m_cheatsEnabled) {
        return;
    }
    if (event.m_type == Event::NONE) {
        return;
    }
    int third = WindowApp::m_instance->m_screenWidth / 3;
    int x = LocalToGlobalX(event.m_param1);
    bool changed;
    if (x < third) {
        signed char tap = TAP_LEFT;
        m_taps.addElement(tap);
        changed = true;
    } else if (x > WindowApp::m_instance->m_screenWidth - third) {
        signed char tap = TAP_RIGHT;
        m_taps.addElement(tap);
        changed = true;
    } else {
        changed = false;
    }
    while (m_taps.size() >= MAX_TAPS) {
        m_taps.removeElementAt(0);
        changed = true;
    }
    if (!changed) {
        return;
    }
    if (CheckCheatTable(CHEAT_LENGTH) == 1) {
        ((App*)WindowApp::m_instance)->m_settings->SetCheatsEnabled(true);
        m_image->SetImage(m_images[IMAGE_ENABLED]);
        m_cheatsEnabled = true;
        return;
    }
    m_image->SetImage(ImageRes());
    for (int length = CHEAT_LENGTH - 1; length >= CHEAT_MIN_PREFIX; length--) {
        if (CheckCheatTable(length) == 1) {
            if (length >= CHEAT_ALMOST_PREFIX) {
                m_image->SetImage(m_images[IMAGE_ALMOST]);
            } else {
                m_image->SetImage(m_images[IMAGE_PARTIAL]);
            }
            return;
        }
    }
}

GameStartSoundDialog::GameStartSoundDialog()
    : YesNoDialog(Window::ResString("IDS_ENABLE_SOUND_TEXT"), 0, 0, 0, NULL)
{
}

void GameStartSoundDialog::Init()
{
    YesNoDialog::Init();
    ICMediaPlayer::GetInstance()->SetSoundEnabled(false);
    AddToFront(new CheatsWindow());
}

void GameStartSoundDialog::Shutdown()
{
    ((App*)WindowApp::m_instance)->m_soundManager->SetActivated(true);
    SimpleDialog::Shutdown();
}

void GameStartSoundDialog::ClickItem(int id, bool byPointer)
{
    AppSettings* settings = ((App*)WindowApp::m_instance)->m_settings;
    if (id == ITEM_YES) {
        settings->SetSoundEnabled(true);
        ((App*)WindowApp::m_instance)->m_settings->SetMusicEnabled(true);
    } else {
        settings->SetSoundEnabled(false);
        ((App*)WindowApp::m_instance)->m_settings->SetMusicEnabled(false);
    }
    ReverseEasy(COMMAND_CLOSE);
}

void GameStartSoundDialog::CancelDialog()
{
    CApplet::m_pCore->ExitApp(com::glu::platform::framework::ICCore::ExitStatus_Normal);
}

SimpleMessage::SimpleMessage(const XString& text, int userValue)
    : YesNoDialog(text, userValue, 0, 0, NULL)
{
    m_textArea->SetFont(CFontMgr::GetInstance()->GetFont(FONT_MESSAGE));
    m_itemsWindow->Close();
    m_itemsWindow = NULL;
    AddItem(ITEM_YES, Window::ResString("IDS_SK_OK"), ITEM_NORMAL);
}
