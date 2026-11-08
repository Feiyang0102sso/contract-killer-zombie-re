// CGameCenterInterface.h - Game Center implementation of CNGSSocialInterface (sizeof 0x48, no own fields).
// Owner: social/ios/CGameCenterInterface.mm (b028x; started as a b016x PROVISIONAL).
// m_nativeManager is the CGameCenterManager (Objective-C, CGameCenterManager.h) doing the GameKit work.
#ifndef CGAMECENTERINTERFACE_H
#define CGAMECENTERINTERFACE_H

#include "CNGSSocialInterface.h"

class CGameCenterInterface : public CNGSSocialInterface {
public:
    CGameCenterInterface();
    virtual ~CGameCenterInterface();

    virtual void login();
    virtual void finishLogin();
    virtual void logout();
    virtual void finishLogout();
    virtual void invalidate();
    virtual void loadFriends();
    virtual void finishLoadFriends();
    virtual int getConnectionState();
    virtual com::glu::platform::components::CStrWChar getPlayerID();
    virtual com::glu::platform::components::CStrWChar getPlayerAlias();
    virtual int getFriendsCount();
    virtual com::glu::platform::components::CStrWChar getFriendIDAtIndex(int index);
    virtual void reportScore(int score, const com::glu::platform::components::CStrWChar& category);
    virtual void finishReportScore();
    virtual void showLeaderboard(void* viewController);
    virtual void showAchievement(void* viewController);
    virtual void showFriendRequestScreen(void* viewController);
    virtual void reportAchievement(int percent, const com::glu::platform::components::CStrWChar& identifier);
    virtual void finishReportAchievement();
    virtual unsigned char requestFriendPicWithID(const com::glu::platform::components::CStrWChar& id,
                                                 const com::glu::platform::components::CStrWChar* type);
    virtual unsigned char getFriendPicWithID(const com::glu::platform::components::CStrWChar& id, const void*& data, unsigned int& size);
    virtual void freeFriendPicWithID(const com::glu::platform::components::CStrWChar& id);
    virtual void freeAllFriendPics();

    /** The Game Center player changed (authentication notification). */
    virtual void handleNotification();
    /** The Game Center friends changed (player data notification). */
    virtual void handleFriendsNotification();
};

#endif
