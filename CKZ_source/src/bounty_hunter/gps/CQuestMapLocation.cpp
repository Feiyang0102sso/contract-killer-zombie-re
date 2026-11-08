// b082: complete quest marker; Update is genuinely return-only in the original.
#include "CQuestMapLocation.h"
#include "CMapLocation.h"
#include "CQuestManager.h"
#include "App.h"

CQuestMapLocation::CQuestMapLocation(CMapLocation* location)
    : CGPSItem(), m_cachedQuestName(), m_listener(NULL), m_location(location), m_cachedMissionName(), m_questId(-1)
{
    SetVisible(false);
    SetScreenName(XString(L""));
    SetPos((float)m_location->m_questX, (float)m_location->m_questY);
}

void CQuestMapLocation::Update(long)
{
    // Original 1483a4 is return-only.
}

void CQuestMapLocation::Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y)
{
    CGPSItem::Paint(graphics, x, y);
}

void CQuestMapLocation::OnClick()
{
    if (m_listener != NULL) {
        m_listener->OnQuestMapLocationSelected(this);
    }
}

CQuestMapLocation::~CQuestMapLocation()
{
}

bool CQuestMapLocation::CheckHit(int x, int y, CGPSItem** hit)
{
    return CGPSItem::CheckHit(x, y, hit);
}

void CQuestMapLocation::SetQuestMapLocationListener(CQuestMapLocationListener* listener)
{
    m_listener = listener;
}

bool CQuestMapLocation::CheckForQuests()
{
    m_questId = -1;
    CQuestManager::SQuest* quest = ((App*)WindowApp::m_instance)->m_questManager->FindAvailableQuestForLocations(m_location->m_locations);
    if (quest != NULL) {
        m_questId = quest->m_id;
        SetScreenName(quest->m_screenName);
        SetSGImage(4, -1);
        SetVisible(true);
        return true;
    }
    SetVisible(false);
    return false;
}
