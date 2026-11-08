// Original u470: seven queue groups, event tips and the original animation parameters; no guards absent from the original are added.
#include "CPopupController.h"
#include "App.h"
#include "ServiceWindows.h"
#include "CAchievementsWindow.h"
#include "CFontMgr.h"
#include "CSwerveGame.h"
#include "DGHelper.h"
#include "CDH_SoundManager.h"
#include "CRandGen.h"
#include "CStdUtil_Cocoa.h"
#include "SScreenAdaptingConstant.h"
using com::glu::platform::core::CRandGen;

const unsigned int CPopupController::POPUP_TIME_INTERVAL_MS[7] = { 500, 1500, 500, 2500, 2500, 7000, 2500 };
const int CPopupController::POPUP_STACK_MAX_SIZE[7] = { 12, 12, 15, 12, 12, 20, 1 };

CPopupController::CPopupController() : WindowTransparent(false)
{
    for (int position = 0; position < 7; ++position) {
        m_lastPopupMS[position] = 0;
        m_queues[position].ensureCapacity(POPUP_STACK_MAX_SIZE[position]);
    }
    // Original 11d824 initializes the two counters only after the seven reserve external calls.
    m_warningDelayMS = 0;
    m_skillshotCount = 0;
    ClearFlags(0x40);
}
CPopupController::~CPopupController() { ClearPopups(false); }

void CPopupController::OnCommand(Event& event)
{
    int previous = m_skillshotCount;
    switch (event.m_id) {
    case (int)0xe3735e88:
        m_skillshotCount = previous + 1;
        AddPopup(NULL, ResString("IDS_POPUP_HEADSHOT"), Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    case 0x6aa69fd2:
        AddPopup(NULL, ResString("IDS_POPUP_LEVELUP"), Position3, AnimationStyle3, true, 16);
        event.Clear();
        break;
    case 0x2e501085:
        if (m_warningDelayMS <= 0) {
            m_warningDelayMS = 5000;
            AddPopup(NULL, ResString("IDS_POPUP_OUT_OF_POWERUPS"), Position3, AnimationStyle1, false, 18);
        }
        event.Clear();
        break;
    case 0x2b46d225:
        if (m_warningDelayMS <= 0) {
            m_warningDelayMS = 5000;
            AddPopup(NULL, ResString("IDS_POPUP_OUT_OF_GRENADES"), Position3, AnimationStyle1, false, 18);
        }
        event.Clear();
        break;
    // The skillshot tip keeps the named text; the argument copy is destroyed first, the text itself only after Event.Clear.
    case (int)0xfab5e72d: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT0");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e72c: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT1");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e72f: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT2");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e72e: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT3");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e729: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT4");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e728: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT5");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e72b: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT6");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    case (int)0xfab5e72a: {
        m_skillshotCount = previous + 1;
        XString text = ResString("IDS_POPUP_SKILLSHOT7");
        AddPopup(NULL, text, Position6, AnimationStyle1, true, 16);
        event.Clear();
        break;
    }
    }
    if (previous < m_skillshotCount) {
        // Draw the random number first even if the current count plays no sound effect, preserving the original side effect.
        float delay = (float)CRandGen::GetInstance()->GetRandRange(1000, 1500) * 0.001f;
        int count = m_skillshotCount;
        if (count % 5 == 0 && count % 15 != 0) {
            ((App*)WindowApp::m_instance)->m_soundManager->PlayRandomSoundWithDelay("KEYSET_SHOOTER_SKILLSHOT_COMMON", delay, false);
        } else if (count % 15 == 0) {
            ((App*)WindowApp::m_instance)->m_soundManager->PlayRandomSoundWithDelay("KEYSET_SHOOTER_SKILLSHOT_RARE", delay, false);
        }
    }
}

void CPopupController::AddPopup(const char* icon, XString text, EPosition position, EAnimationStyle style, bool animated, int fontId)
{
    AnimatedWindow* window = new AnimatedWindow((AnimatedWindow::EMode)2);
    // The original allocates the Label first, then copies the text argument, then queries FontMgr; keep the same new full-expression.
    LabelWithIcon* label = new LabelWithIcon(icon, text, false, CFontMgr::GetInstance()->GetFont(fontId), 0);
    window->AddToFront(label);
    SetupStyle(window, style);
    SetupPosition(window, position);
    if (animated) {
        CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
        if (game != NULL && game->AreAnimatedHitPointsSupported() && position == Position6) {
            WindowTransparent* parent = new WindowTransparent(false);
            parent->SetWidthByContent(0, 0);
            parent->SetHeightByContent(0, 0);
            parent->SetLayoutType(Window::LAYOUT_SIMPLE);
            parent->AddToFront(window);
            window->SetAlign(36);
            ((App*)WindowApp::m_instance)->m_helper->m_swerveGame->AddSkillshot(parent);
            return;
        }
    }
    AddPopupInQueue(window, position);
}
void CPopupController::AddAchievementPopup(const CAchievement* achievement)
{
    AnimatedWindow* window = new AnimatedWindow((AnimatedWindow::EMode)2);
    AchievementReceivedWindow* content = new AchievementReceivedWindow(achievement);
    window->AddToFront(content);
    SetupStyle(window, AnimationStyle4);
    SetupPosition(window, Position5);
    AddPopupInQueue(window, Position5);
}
void CPopupController::SetupStyle(AnimatedWindow* window, EAnimationStyle style)
{
    switch (style) {
    case AnimationStyle0:
        window->SetStartEasy(0, 0, 0.25f, 0.0f, 1.0f, 0.0f, (EasyLib::EMethod)0);
        window->SetCloseEasy(0, 0, 0.5f, 1.0f, 0.0f, 1.5f, (EasyLib::EMethod)0);
        window->SetCloseEasyScale(1.0f, 7.0f);
        break;
    case AnimationStyle1:
        window->SetStartEasy(0, 0, 0.35f, 0.0f, 1.0f, 0.0f, (EasyLib::EMethod)18);
        window->SetStartEasyScale(20.0f, 1.0f);
        window->SetCloseEasy(0, 0, 0.7f, 1.0f, 0.0f, 1.5f, (EasyLib::EMethod)18);
        break;
    case AnimationStyle2:
        window->SetStartEasy(0, App::Scale() * 20, 0.5f, 0.0f, 1.0f, 0.0f, (EasyLib::EMethod)0);
        window->SetCloseEasy(0, App::Scale() * -40, 0.5f, 1.0f, 0.0f, 0.5f, (EasyLib::EMethod)0);
        break;
    case AnimationStyle3:
        App::Scale(); // Original 11e02c really keeps this unused query.
        window->SetStartEasy(0, 0, 0.55f, 0.0f, 0.7f, 0.0f, (EasyLib::EMethod)0);
        window->SetCloseEasy(0, App::Scale() * -70, 0.3f, 0.7f, 0.0f, 0.05f, (EasyLib::EMethod)0);
        break;
    case AnimationStyle4:
        window->SetStartEasy(0, App::Scale() * 70, 0.5f, 0.0f, 1.0f, 0.0f, (EasyLib::EMethod)26);
        window->SetCloseEasy(0, 0, 2.0f, 1.0f, 0.0f, 4.0f, (EasyLib::EMethod)0);
        break;
    case AnimationStyle5:
        window->SetStartEasy(0, 0, 0.35f, 0.0f, 1.0f, 0.0f, (EasyLib::EMethod)18);
        window->SetStartEasyScale(10.0f, 1.0f);
        window->SetCloseEasy(0, 0, 0.7f, 1.0f, 0.0f, 1.5f, (EasyLib::EMethod)18);
        window->SetCloseEasyScale(1.0f, 10.0f);
        break;
    }
}
void CPopupController::SetupPosition(AnimatedWindow* window, EPosition position)
{
    switch (position) {
    case Position0: {
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(10);
        int top = SScreenAdaptingConstant(45, 90)();
        int right = SScreenAdaptingConstant(20, 40)();
        window->SetOutsetSpacing(top, right, 0, 0);
        break;
    }
    case Position1: {
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(34);
        int top = SScreenAdaptingConstant(20, 40)();
        int right = SScreenAdaptingConstant(20, 40)();
        window->SetOutsetSpacing(top, right, 0, 0);
        break;
    }
    case Position2: {
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(17);
        int bottom = SScreenAdaptingConstant(50, 100)();
        int left = SScreenAdaptingConstant(60, 120)();
        window->SetOutsetSpacing(0, 0, bottom, left);
        break;
    }
    case Position4: {
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(36);
        int top = SScreenAdaptingConstant(20, 20, 0)();
        int right = SScreenAdaptingConstant(20, 40, 30)();
        window->SetOutsetSpacing(top, right, 0, 0);
        break;
    }
    case Position3:
    case Position6: {
        int height = ((App*)WindowApp::m_instance)->m_helper->m_screenHeight;
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(12);
        window->SetOutsetSpacing(SScreenAdaptingConstant(15)(), 0, 0, 0);
        CSwerveGame* game = ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
        if (game != NULL && game->AreAnimatedHitPointsSupported()) return;
        int multiplier = 1;
        if (position == Position6) multiplier = 2;
        int offset = SScreenAdaptingConstant(60, 120)() * multiplier;
        int bottom = SScreenAdaptingConstant(20, 40)() * multiplier;
        window->SetOutsetSpacing(height / 2 - offset, 0, bottom, 0);
        break;
    }
    case Position5:
        window->SetLayoutType(Window::LAYOUT_SIMPLE);
        window->SetAlign(20);
        window->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(0)(), 0);
        break;
    }
}
void CPopupController::AddPopupInQueue(Window* window, EPosition position)
{
    int maximum = POPUP_STACK_MAX_SIZE[position];
    Vector<Window*>& queue = m_queues[position];
    int size = queue.size();
    if (position == Position6) {
        if (size >= maximum && size > 0) {
            Window* last = queue[size - 1];
            last->CloseChilds();
            last->HandleUpdate(true);
            last->HandleUpdate(true);
            last->Close();
            queue.removeElementAt(maximum - 1);
        }
    } else if (size > maximum) {
        window->CloseChilds();
        window->HandleUpdate(true);
        window->HandleUpdate(true);
        window->Close();
        return;
    }
    queue.insertElementAt(window, 0);
}
void CPopupController::AddTextPopup(const XString& text, EPosition position, EAnimationStyle animation, bool flag, int fontId)
{
    AddPopup(NULL, text, position, animation, flag, fontId);
}
void CPopupController::ClearPopups(bool immediate)
{
    for (int position = 0; position < 7; ++position) {
        Vector<Window*>& queue = m_queues[position];
        for (int index = 0; index < queue.size(); ++index) {
            Window* window = queue[index];
            window->CloseChilds();
            window->HandleUpdate(true);
            window->HandleUpdate(true);
            window->Close();
            if (immediate) window->ClearFlags(1);
        }
        queue.removeAllElements();
    }
}
void CPopupController::Update()
{
    for (int position = 0; position < 7; ++position) {
        Vector<Window*>& queue = m_queues[position];
        if (queue.size() > 0) {
            unsigned int time = CStdUtil_Cocoa::GetUpTimeMS();
            if (time - m_lastPopupMS[position] > POPUP_TIME_INTERVAL_MS[position]) {
                ((App*)WindowApp::m_instance)->m_mainLayer->AddToFront(queue[queue.size() - 1]);
                queue.removeElementAt(queue.size() - 1);
                m_lastPopupMS[position] = time;
            }
        }
    }
    if (m_warningDelayMS > 0) m_warningDelayMS -= WindowApp::m_instance->m_updateFrameTime;
}
