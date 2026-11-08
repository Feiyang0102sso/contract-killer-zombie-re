// PROVISIONAL (created by b057x): owner src/bounty_hunter/CHuntingInfo.cpp not written yet
// CHuntingInfo.h - hunting statistics texts; only what src/general/App.cpp uses is declared.
// b074i: complete owner restored per the original range; the PROVISIONAL above is a historical record.
#ifndef CHUNTINGINFO_H
#define CHUNTINGINFO_H

#include <stddef.h>
#include "xString.h"

/** Values proven by Scene LoseAfterDelay/WinAfterDelay; enumerator names are descriptive. */
enum EBountyResult {
    BountyResultPending = 0,
    BountyResultLose = 1,
    BountyResultWin = 3
};

class CHuntingInfo {
public:
    CHuntingInfo();
    // b067h: complete original Game call; this declaration does not pose as the restored HuntingInfo body.
    void SetBountyResult(EBountyResult result); // 0x11ccb0
    bool IsPlayerWin() const; // 0x11cbf8
    void AddArcadePoints(int points, int experience, int cash); // 0x11cccc
    /** The assigned hunt (36 bytes; also stored at CHuntingInfo+0x14). */  // added by b055x
    struct SAssignedData {  // added by b055x
        SAssignedData();  // added by b055x
  // added by b055x
        XString m_missionName;      // 0x00  // added by b055x
        int m_questId;              // 0x04 -1: none  // added by b055x
        int m_field08;              // 0x08 -1  // added by b055x
        XString m_text0c;           // 0x0c  // added by b055x
        XString m_text10;           // 0x10  // added by b055x
        int m_field14;              // 0x14  // added by b055x
        int m_field18;              // 0x18  // added by b055x
        XString m_text1c;           // 0x1c  // added by b055x
        bool m_flag20;              // 0x20  // added by b055x
        bool m_flag21;              // 0x21  // added by b055x
    };  // added by b055x
  // added by b055x
    void SetHuntingData(SAssignedData data);  // added by b055x
    /** Bookkeeping after the 3D scene closed. */  // added by b055x
    void CleanHuntingStatistic();
    void OnEnemyKilled();
    void OnEnemyTranqullized();
    void OnEnemyEscaped();
    void AfterSceneActivity();  // added by b055x
    /** Mission bookkeeping offsets from CMission::OnMissionStarted/Completed. */  // added by b078b
    int& ProvisionToSend() { return m_assignedData.m_field14; }
    int& PeopleToSend() { return m_assignedData.m_field18; }
    int PeopleEarned() const { return m_peopleEarned; }
    int ProvisionEarned() const { return m_provisionEarned; }
    int PeopleSent() const { return m_peopleSent; }
    void SetPeopleEarned(int value) { m_peopleEarned = value; }
    void SetProvisionEarned(int value) { m_provisionEarned = value; }
    void SetPeopleSent(int value) { m_peopleSent = value; }
    /** Read by original Unit102882/1028a4, 102c0c/102c28; the concrete meaning of the counter awaits recovery of this owner. */
    int GetCounter5c() const { return m_counter5c; }

    bool m_field00;                 // 0x00 restore-scene flag, only one byte is written
    unsigned char m_padding01[3];
    XString m_text04;               // 0x04, original explicit XString(0)
    int m_arcadePoints;             // 0x08
    int m_arcadeExperience;         // 0x0c
    int m_arcadeMoney;              // 0x10
    SAssignedData m_assignedData;   // 0x14, real embedded 36 bytes
    EBountyResult m_result;         // 0x38
    bool m_flag3c;                 // 0x3c
    unsigned char m_padding3d[3];
    int m_peopleEarned;             // 0x40
    int m_provisionEarned;          // 0x44
    int m_field48;                 // 0x48
    int m_field4c;                 // 0x4c
    int m_peopleSent;               // 0x50
    int m_field54;                 // 0x54
    int m_scoreAtStart;             // 0x58
    int m_counter5c;                // 0x5c
    int m_enemiesKilled;            // 0x60
    int m_enemiesTranquillized;     // 0x64
    int m_enemiesEscaped;           // 0x68
    bool m_levelIncreased;          // 0x6c
    unsigned char m_padding6d[3];
    int m_currentWave;              // 0x70
    float m_civiliansRelativeHp;    // 0x74

#if 0 // The historical thin layout and all of the user's original comments are kept; the real fields are above.
    int m_field00;                  // 0x00
    XString m_text04;               // 0x04
    unsigned char m_data08[0x14 - 0x08];
    XString m_text14;               // 0x14
    unsigned char m_data18[0x20 - 0x18];
    XString m_text20;               // 0x20
    XString m_text24;               // 0x24
    unsigned char m_data28[0x30 - 0x28];
    XString m_text30;               // 0x30
    unsigned char m_data34[0x78 - 0x34];
#endif
};

#endif
