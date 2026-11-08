// PROVISIONAL (created by b048): owner src/bounty_hunter/CGPSMapGame.cpp not written yet
// CGPSMapGame.h - the map game; only what src/dh3d uses is declared.
#ifndef CGPSMAPGAME_H
#define CGPSMAPGAME_H

#include "CBH_GPSMap.h"
#include "CMapLocation.h"
#include "CQuestMapLocation.h"
#include "CShopWindow.h"
#include "TimeValue.h"
#include "CSharedPtr.h"

class CLoadingScreen;
class CBH_GPSHud;
class TrackHandler;
struct SMessage;

// b074i: the real dual-listener secondary base classes are at 168/16c; the historical thin-view comments are kept.
class CGPSMapGame : public CBH_GPSMap, public CMapLocationListener,
    public CQuestMapLocationListener {
public:
    class TapjoyUpdater : public TimeManager::Action {
    public:
        explicit TapjoyUpdater(CGPSMapGame* game) : m_game(game) {}
        virtual ~TapjoyUpdater() {}
        virtual int IsDestructible() const { return 0; }
        virtual void Execute(TimeManager* manager);
        void onReceiveTapjoyPoints(int points);
        CGPSMapGame* m_game;
    };
    CGPSMapGame();
    virtual ~CGPSMapGame();
    virtual void Init();
    virtual void OnGetFocus();
    virtual void Update();
    virtual void OnCommand(Event& event);
    // Declaration order comes from the original primary vtable c0/c4/c8/cc; must not be reordered by base class order.
    virtual void OnMapLocationSelected(CMapLocation* location);
    virtual void OnQuestMapLocationSelected(CQuestMapLocation* location);
    virtual void OnShopItemSelected(CGPSShop* shop);
    virtual void OnSafehouseItemSelected(CGPSSafehouse* safehouse);
    void AddDialogInQueue(Window* dialog); // added by b078c
    void AddLevelUpDialogInQueue(int unused);
    void CreateMapLocations();
    void ShowShop(CShopWindow::ETab tab, int itemId, bool tapjoy);
    void ShowQuestFinalDialogs(int questId);
    void StartNewCycle();
    void PopulateMapLocationsFromPool();
    void PopulateMapLocationsWithEndlessModeMission();
    void PopulateTutorialLocations();
    void SearchForAvailableQuests();
    void AddRecommendedWeaponDialogInQueue();
    void UpdateTutorialAttractions();
    void ShowCompleteQuestFirstDialog();
    void ShowMissionStartDialog(XString missionName, int descriptionId);
    void SetMapLocationsVisible(bool visible, CMission::EMissionType type);
    void ClearLocations();
    void PopulateMapLocationsWithKillAllMission();
    CMapLocation* LocationByName(const XString& name);
    void ShowZombieDialog(XString title, XString text);
    void ResetBriefingSound();
    void StartHunt(const CMission* mission);
    void StartMission(const CMission* mission);

#if 0 // The original padding was computed from ac; no longer used after adding the real base classes, original comment kept.
    unsigned char m_dataac[0x1fc - 0xac];   // layout not known yet (size from CGPSMapScreen::Init)
#endif
    Vector<CMapLocation*> m_locations;       // 170, entities owned by the Window tree.
    Vector<CMapLocation*> m_extraLocations;  // 180, only traversed by ClearLocations in the original.
    Vector<CQuestMapLocation*> m_questLocations; // 190
    int m_selectedQuestId;                   // 1a0
    CMapLocation* m_selectedLocation;         // 1a4
    CQuestMapLocation* m_availableQuest;      // 1a8
    int m_selectedFriendId;                  // 1ac
    XString m_selectedFriendName;            // 1b0
    const SMessage* m_targetMessage;          // 1b4
    int m_targetMessageIndex;                // 1b8
    CBH_GPSHud* m_hud;                       // 1bc
    TapjoyUpdater m_tapjoyUpdater;           // 1c0
    TimeManager m_timer;                     // 1c8, total 2c
    com::glu::platform::core::CSharedPtr<TrackHandler> m_briefingTrack; // 1f4
};

/** Queue the loading tasks of `game` on `screen`. */
void FillMapLoadingTasks(CLoadingScreen* screen, CGPSMapGame* game);
extern const int EventId_StartTargettingMission;

#endif
