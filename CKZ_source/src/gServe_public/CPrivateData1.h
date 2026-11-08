// CPrivateData1.h - the game's private profile collection (currencies, experience and the
// settings that are synced with gServe). (created by b047x, owner of CPrivateData1.cpp)
// Only the getters live in CPrivateData1.cpp; the virtual functions (and the constructor) are
// defined elsewhere / were dead-stripped, so this unit emits no vtable.
#ifndef CPRIVATEDATA1_H
#define CPRIVATEDATA1_H

#include "CSaveRestoreInterface.h"

class CPrivateData1 : public CSaveRestoreInterface {
public:
    /** +0x1c human readable summary of the collection (shown in the data conflict dialog). */
    virtual com::glu::platform::components::CStrWChar getDescription();

    long long getSoftCurrency();
    long long getHardCurrency();
    long long getExperience();
    unsigned char getSoundSetting();
    unsigned char getMusicSetting();
    unsigned char getFireBtnSetting();
    unsigned char getNotificationSetting();

    short m_progress;              // 0x20 0 = nothing played yet (HandleProfileDataConflict)
    long long m_softCurrency;       // 0x24 (long long is 4 byte aligned on iOS ARM)
    long long m_hardCurrency;       // 0x2c
    long long m_experience;         // 0x34
    int m_field3c;                  // 0x3c not used by this module
    unsigned char m_soundSetting;           // 0x40
    unsigned char m_musicSetting;           // 0x41
    unsigned char m_fireBtnSetting;         // 0x42
    unsigned char m_notificationSetting;    // 0x43
};

#endif
