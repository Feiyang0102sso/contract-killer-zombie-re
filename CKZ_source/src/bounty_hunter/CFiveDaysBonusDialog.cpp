// b074: original 11a0ec..11a844; preserves the externally callable sixth icon branch.
#include "CFiveDaysBonusDialog.h"
#include "SScreenAdaptingConstant.h"
#include "CFiveDaysBonus.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SGImageWindow.h"
#include "ServiceWindows.h"
#include "App.h"

CFiveDaysBonusDialog::CFiveDaysBonusDialog(int day) : CZombieDialogWindow(2)
{
    m_bonusContent = new Window(false);
    m_bonusContent->SetLayoutType(LAYOUT_SIMPLE);
    m_bonusContent->SetPercentHeight(60, 0, 0);
    m_bonusContent->SetPercentWidth(95, 0, 0);
    AddToFront(m_bonusContent);
    m_bonusContent->SetAlign(0xc);
    m_bonusContent->SetOutsetSpacing(SScreenAdaptingConstant(10, 40)(), 0, 0, 0);
    int horizontal = SScreenAdaptingConstant(30, 60)();
    int vertical = SScreenAdaptingConstant(30)();
    SetInsetSpacing(horizontal, vertical);
    CreateTitle();
    CreateMessage(day);
    CreateIcons(day);
    AddButton(BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
}

void CFiveDaysBonusDialog::CreateTitle()
{
    SetTitle(Window::ResString("IDS_DAILY_BONUS_TITLE"));
}

void CFiveDaysBonusDialog::CreateMessage(int day)
{
    Window* container = new Window(false);
    container->SetPercentWidth(100, 0, 0);
    container->SetHeightByContent(0, 0);
    container->SetOutsetSpacing(SScreenAdaptingConstant(0)(), 0, 0, 0);
    XString bonusName = ((App*)WindowApp::m_instance)->m_fiveDaysBonus->GetDayBonusNameForMessage(day);
    XString daysText;
    if (day == 0) {
        daysText.Assign(Window::ResString("IDS_FIVE_DAYS_BONUS_FIRST_DAY"));
    } else {
        // The format string is a temporary of the same full expression (original slot order).
        daysText.Assign(XString::Format(Window::ResString("IDS_DAILY_BONUS_TEXT_DAYS").c_str(), day + 1));
    }
    XString bonusText = XString::Format(Window::ResString("IDS_DAILY_BONUS_BONUS").c_str(), bonusName.c_str());
    TextWindow* days = new TextWindow();
    TextWindow* bonus = new TextWindow();
    com::glu::platform::graphics::CFont* daysFont = CFontMgr::GetInstance()->GetFont(24);
    com::glu::platform::graphics::CFont* bonusFont = CFontMgr::GetInstance()->GetFont(28);
    days->SetText(daysText, daysFont);
    bonus->SetText(bonusText, bonusFont);
    days->SetAlign(0x24);
    bonus->SetAlign(0x24);
    days->SetHeightByContent(0, 0);
    days->SetPercentWidth(85, 0, 0);
    bonus->SetPercentWidth(80, 0, 0);
    container->AddToFront(createTD(days, 3), 0, 0);
    container->AddToFront(createTD(bonus, 3), 0, 1);
    bonus->SetHeightByContent(0, 0);
    bonus->SetOutsetSpacing(0, 0, 0, 0);
    // Base dialog +b8: the actual owner supplies this container field.
    m_messageContainer->AddToFront(container, 0, 0);
    m_messageContainer->SetAlign(0x14);
    m_messageContainer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(45)(), 0);
}

void CFiveDaysBonusDialog::CreateIcons(int day)
{
    m_icons = new Window(false);
    m_icons->SetLayoutType(LAYOUT_SIMPLE);
    m_icons->SetAlign(0x24);
    m_icons->SetWidthByContent(0, 0);
    m_icons->SetHeightByContent(0, 0);
    m_icons->SetOutsetSpacing(0, 0, 0, 0);
    m_bonusContent->AddToFront(m_icons, 0, 1);
    for (int index = 0; index < 5; ++index) {
        CreateIcon(index, day);
    }
}

void CFiveDaysBonusDialog::CreateIcon(int index, int currentDay)
{
    TextWindow* day = new TextWindow();
    ImageWindow* image = new ImageWindow();
    XString dayText = XString::Format(L"Day %d", index + 1);
    com::glu::platform::graphics::CFont* previousFont = CFontMgr::GetInstance()->GetFont(29);
    com::glu::platform::graphics::CFont* currentFont = CFontMgr::GetInstance()->GetFont(28);
    com::glu::platform::graphics::CFont* futureFont = CFontMgr::GetInstance()->GetFont(27);
    com::glu::platform::graphics::CFont* font = previousFont;
    if (index >= currentDay) {
        font = futureFont;
        if (index == currentDay) {
            font = currentFont;
        }
    }
    day->SetText(dayText, font);
    day->SetAlign(0x24);
    bool showName = true;
    if (index == 5) {
        image->SetImageById(((App*)WindowApp::m_instance)->m_fiveDaysBonus->GetDayBonusIcon(5));
        image->SetAlign(0x24);
        if (currentDay > 5) {
            showName = false;
        }
    } else if (index < currentDay) {
        image->SetImageById("IDB__PASSED_DAY_PNG");
        image->SetAlign(0x24);
        showName = false;
    } else {
        image->SetImageById(((App*)WindowApp::m_instance)->m_fiveDaysBonus->GetDayBonusIcon(index));
        image->SetAlign(0x24);
    }
    TextWindow* name = NULL;
    if (showName) {
        name = new TextWindow();
        name->SetText(((App*)WindowApp::m_instance)->m_fiveDaysBonus->GetDayBonusNameForIcon(index), font);
        name->SetAlign(0x24);
        name->SetWidthByContent(0, 0);
        name->SetHeightByContent(0, 0);
        if (index == currentDay) {
            SGImageWindow* animation = new SGImageWindow();
            animation->SetArchetypeCharacter(1, 0);
            animation->SetAnimation(67, 1);
            animation->SetAlign(0x24);
            name->AddToFront(animation);
        }
    }
    Window* wrapper = new Window(false);
    wrapper->AddToFront(image);
    wrapper->SetAlign(0x24);
    wrapper->SetWidthByContent(0, 0);
    wrapper->SetHeightByContent(0, 0);
    int horizontal = SScreenAdaptingConstant(-10)();
    int vertical = SScreenAdaptingConstant(-10)();
    wrapper->SetOutsetSpacing(horizontal, vertical);
    m_icons->AddToFront(day, index, 0);
    m_icons->AddToFront(wrapper, index, 1);
    if (name != NULL) {
        m_icons->AddToFront(name, index, 2);
    }
}
