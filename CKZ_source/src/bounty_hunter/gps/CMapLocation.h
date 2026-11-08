// PROVISIONAL (created by b078b): owner src/bounty_hunter/gps/CMapLocation.cpp not written yet
#ifndef CMAPLOCATION_H
#define CMAPLOCATION_H
#include "xString.h"
#include "Vector_ss.h"
#include "CGPSItem.h"
#include "CMission.h"
/** Only the location-name list used at 0x12d148 is mapped; base/vtable remain unresolved. */
// b082: historical partial view above is superseded by the actual base/fields below.
class CMapLocation;
class CMapLocationListener {
public:
    virtual void OnMapLocationSelected(CMapLocation* location) = 0;
};
class TiXmlNode;
class CMapLocation : public CGPSItem {
public:
    struct SLocationMission {
        XString m_name;
        int m_descriptionId;
    };
    CMapLocation(TiXmlNode* node);
    virtual ~CMapLocation();
    virtual void Update(long elapsed);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void OnClick();
    virtual bool CheckHit(int x, int y, CGPSItem** hit);
    virtual void Save(TiXmlElement* element);
    virtual void Load(TiXmlElement* element);
    void ParseNode(TiXmlNode* node);
    void SetMapLocationListener(CMapLocationListener* listener);
    void SetMission(const XString& name, bool clear);
    void UpdateCurrentMission();
    XString GetCurrentMission();
    int GetCurrentMissionDescriptionId();
    bool HasMission();
    void ClearMissions();
    void CompleteMission();
    // Former m_data00[0xd4] included a real CGPSItem base and this id.
    int m_id; // d0
    Vector<XString> m_locations; // 0xd4
    CMapLocationListener* m_listener; // e4
    Vector<SLocationMission> m_missions; // e8
    int m_unresolvedF8; // f8 initialized -1, no access in this owner
    bool m_hasQuestImage; // fc
    int m_questX; // 100
    int m_questY; // 104
    XString m_questImage; // 108
    bool m_withFriend; // 10c
    CMission::EMissionType m_missionType; // 110; complete size114
};
#endif
