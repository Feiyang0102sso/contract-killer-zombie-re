// b082: complete quest marker ABI, original 1482e4 and vtable4deb60.
#ifndef CQUESTMAPLOCATION_H
#define CQUESTMAPLOCATION_H
#include "CGPSItem.h"
class CMapLocation;
class CQuestMapLocation;
class CQuestMapLocationListener {
public:
    virtual void OnQuestMapLocationSelected(CQuestMapLocation* location) = 0;
};
class CQuestMapLocation : public CGPSItem {
public:
    CQuestMapLocation(CMapLocation* location);
    virtual ~CQuestMapLocation();
    virtual void Update(long elapsed);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void OnClick();
    virtual bool CheckHit(int x, int y, CGPSItem** hit);
    void SetQuestMapLocationListener(CQuestMapLocationListener* listener);
    bool CheckForQuests();
    XString m_cachedQuestName; // d0, ctor-empty; currently no writer in this owner
    CQuestMapLocationListener* m_listener; // d4
    CMapLocation* m_location; // d8
    XString m_cachedMissionName; // dc, ctor-empty
    int m_questId; // e0, complete sizee4
};
#endif
