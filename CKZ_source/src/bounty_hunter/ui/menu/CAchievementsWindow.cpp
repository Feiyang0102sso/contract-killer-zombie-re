// u553 complete three windows; the percentage multiplication first wraps in 32 bits, then is computed as float.
#include "CAchievementsWindow.h"
#include "CAchievementManager.h"
#include "CGameAnalytics.h"
#include "CSettingsWindow.h"
#include "CImageButton.h"
#include "CFontMgr.h"
#include "BaseDialog.h"
#include "ServiceWindows.h"
#include "SScreenAdaptingConstant.h"

static SScreenAdaptingConstant SCROLLER_SPACING(10, 20);
static SScreenAdaptingConstant ITEM_SPACING(5, 10);

CAchievementsWindow::CAchievementsWindow() : CBaseScreen(true, true, true)
{
    CGameAnalytics::logMenuAccessed(XString(L"Awards"));
    m_achievementsContainer = new Window(false);
    m_achievementsContainer->SetLayoutType(LAYOUT_SIMPLE);
    m_achievementsContainer->SetAlign(36);
    m_achievementsContainer->SetPercentHeight(80, 0, 0);
    m_achievementsContainer->SetWidthByContent(0, 0);
    ScrollingContainer* scrolling = new ScrollingContainer;
    scrolling->SetCellPos(0, 0, 1, 1);
    scrolling->SetAlign(36);
    scrolling->SetDesiredWidth((int)((float)WindowApp::m_instance->m_screenWidth * 0.95f));
    Window* list = new WindowTransparent(false);
    list->SetLayoutType(LAYOUT_SIMPLE);
    list->SetHeightByContent(0, 0);
    list->SetWidthByContent(0, 0);
    list->SetAlign(36);
    int topScale = App::Scale();
    int leftScale = App::Scale();
    list->SetInsetSpacing(topScale * 10, 0, 0, leftScale * 5);
    CAchievementManager* manager = ((App*)WindowApp::m_instance)->m_achievementManager;
    int row = 0;
    for (int index = 0; index < manager->m_names.size(); ++index) {
        CAchievement* achievement = *manager->m_achievements.Get(manager->m_names[index]);
        if (achievement->m_progress != 0 || !achievement->m_hidden) {
            AchievementInfoWindow* info = new AchievementInfoWindow(achievement);
            info->SetOutsetSpacing(0, 0, ITEM_SPACING(), 0);
            list->AddToFront(info, 0, row);
            ++row;
        }
        manager = ((App*)WindowApp::m_instance)->m_achievementManager;
    }
    Window* wrapper = new WindowTransparent(false);
    wrapper->SetPercentWidth(100, 0, 0);
    wrapper->SetHeightByContent(0, 0);
    wrapper->SetLayoutType(LAYOUT_NONE);
    wrapper->AddToFront(list);
    scrolling->AddToFront(wrapper);
    m_achievementsContainer->AddToFront(scrolling);
    ScrollingContainer::VertScroller* scroller = new ScrollingContainer::VertScroller(scrolling);
    scroller->SetOutsetSpacing(0, 0, 0, SCROLLER_SPACING());
    scroller->SetCellPos(1, 0, 1, 1);
    m_achievementsContainer->AddToFront(scroller);
    Window* container = createTD(m_achievementsContainer, 3);
    AddToFront(container, 0, 0);
    container->SetPercentHeight(92, 0, 0);
    ClearFlags(0x40);
}

void CAchievementsWindow::OnCommand(Event& event)
{
    // b147: original places the back (0x97204784) block first, so it is written first.
    if (event.m_id == (int)0x97204784) {
        ReverseEasy(0x200);
        event.Clear();
    } else if (event.m_id == (int)0x9720b34a) {
        CSettingsWindow* settings = new CSettingsWindow(false, false);
        settings->SetLayoutType(LAYOUT_SIMPLE);
        settings->SetAlign(36);
        ((App*)WindowApp::m_instance)->m_mainLayer->AddModal(settings);
        event.Clear();
    }
    CBaseScreen::OnCommand(event);
}

AchievementInfoWindow::AchievementInfoWindow(const CAchievement* achievement)
{
    static const wchar_t* PercentageExclusionPattern[4] = {
        L"DAILY_BONUSES", L"REACH_LEVEL", L"EM_PROTECT", L"EM_WAVES"
    };
    SetArchetypeCharacter(7, 0);
    SetAnimation(1, 1);
    int progress = achievement->m_progress;
    int steps = achievement->m_steps;
    int multiplied = (int)((unsigned int)achievement->m_currentValue * 100u);
    int percent = (int)((float)multiplied / (float)achievement->m_value);
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }
    if (progress < steps) {
        SetOpacity((float)percent * 0.0075f + 0.25f);
    }
    Window* content = new WindowTransparent(false);
    int top = SScreenAdaptingConstant(3, 10)();
    App::Scale(); // Original 1536ea really has this unused query; do not delete it.
    int bottom = SScreenAdaptingConstant(20, 10)();
    int left = SScreenAdaptingConstant(10, 20)();
    content->SetInsetSpacing(top, 0, bottom, left);
    XString title = achievement->m_screenName;
    if (progress == steps) {
        ImageWindow* image = new ImageWindow;
        top = SScreenAdaptingConstant(7, 0)();
        left = SScreenAdaptingConstant(0)();
        image->SetOutsetSpacing(top, 0, 0, left);
        image->SetAlign(33);
        {
            XString::AnsiString id(achievement->m_imageId.m_data);
            image->SetImageById(id);
        }
        content->AddToFront(image);
    } else {
        bool excluded = false;
        for (int index = 0; index != 4; ++index) {
            if (achievement->m_name.Find(PercentageExclusionPattern[index], 0) != -1) {
                if (percent < 100) {
                    SetOpacity(0.25f);
                }
                excluded = true;
                break;
            }
        }
        if (!excluded && percent > 0) {
            XString percentage = XString(percent) + XString("% ");
            TextWindow* text = new TextWindow(percentage.Upper(), CFontMgr::GetInstance()->GetFont(29));
            text->SetAlign(33);
            top = SScreenAdaptingConstant(10, 0)();
            left = SScreenAdaptingConstant(7)();
            text->SetOutsetSpacing(top, 0, 0, left);
            content->AddToFront(text);
        }
    }
    TextWindow* titleText = new TextWindow(title.Upper(), CFontMgr::GetInstance()->GetFont(29));
    titleText->SetAlign(36);
    titleText->SetPercentWidth(83, 0, 0);
    content->AddToFront(titleText, 0, 0);
    const XString* description = &achievement->m_description;
    if (progress == steps) {
        description = &achievement->m_completedDescription;
    }
    TextWindow* descriptionText = new TextWindow(*description, CFontMgr::GetInstance()->GetFont(24));
    top = SScreenAdaptingConstant(-2, 0)();
    left = SScreenAdaptingConstant(20, 0)();
    descriptionText->SetOutsetSpacing(top, 0, 0, left);
    descriptionText->SetAlign(36);
    descriptionText->SetPercentWidth(90, 0, 0);
    content->AddToFront(createTD(descriptionText, 0, 1, 3));
    content->SetPercentWidth(100, 0, 0);
    content->SetPercentHeight(100, 0, 0);
    AddToFront(content);
}

AchievementReceivedWindow::AchievementReceivedWindow(const CAchievement* achievement)
{
    SetArchetypeCharacter(7, 0);
    SetAnimation(1, 1);
    int progress = achievement->m_progress;
    int steps = achievement->m_steps;
    Window* unusedContainer = new WindowTransparent(false);
    XString title = achievement->m_screenName;
    if (progress == steps) {
        CImageButton* image = new CImageButton;
        {
            XString::AnsiString id(achievement->m_imageId.m_data);
            image->AddImage(id, 0, 0);
        }
        image->SetLayoutType(LAYOUT_SIMPLE);
        image->SetAlign(33);
        image->SetWidthByContent(0, 0);
        image->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(10)());
        AddToFront(image);
    } else {
        int multiplied = (int)((unsigned int)progress * 100u);
        int percent = (int)((float)multiplied / (float)steps);
        title = XString(percent) + XString("%  ") + title;
    }
    TextWindow* text = new TextWindow(ResString("IDS_ACHIEVEMENT_UNLOCKED") + title, CFontMgr::GetInstance()->GetFont(29));
    text->SetAlign(36);
    text->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(30)());
    AddToFront(text);
    AddToFront(unusedContainer); // Original 153ccc really attaches an empty container; do not clean up this seemingly redundant node.
}
