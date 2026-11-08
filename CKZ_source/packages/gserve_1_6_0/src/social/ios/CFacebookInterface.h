// CFacebookInterface.h - Facebook implementation of CNGSSocialInterface (sizeof 0x48, no own fields):
// m_nativeManager is the CFacebookManager doing the Facebook SDK work.
// Owned by social/ios/CFacebookInterface.mm (b027, written by b027x). Objective-C++ only.
#ifndef CFACEBOOKINTERFACE_H
#define CFACEBOOKINTERFACE_H

#import <Foundation/Foundation.h>
#include "CNGSSocialInterface.h"

class CFacebookInterface : public CNGSSocialInterface {
public:
    CFacebookInterface();
    virtual ~CFacebookInterface();

    /** Converts the app id to UTF-8 and creates the Facebook session object (the secret is unused). */
    virtual void setAppID(com::glu::platform::components::CStrWChar appId, com::glu::platform::components::CStrWChar appSecret);
    virtual void login();
    virtual void finishLogin();
    virtual void logout();
    virtual void finishLogout();
    virtual void invalidate();
    virtual void loadFriends();
    virtual void finishLoadFriends();
    virtual int getConnectionState();
    virtual int getPostConnectionState();
    virtual com::glu::platform::components::CStrWChar getPlayerID();
    virtual com::glu::platform::components::CStrWChar getPlayerAlias();
    virtual com::glu::platform::components::CStrWChar getPlayerEmail();
    virtual int getFriendsCount();
    virtual com::glu::platform::components::CStrWChar getFriendIDAtIndex(int index);
    virtual void resume();
    virtual void postMessage(CNGSSocialMessage* message);
    virtual void postMessage(const com::glu::platform::components::CStrWChar* text, const CNGSUser* targetUser,
                             const com::glu::platform::components::CStrWChar* link,
                             const com::glu::platform::components::CStrWChar* name,
                             const com::glu::platform::components::CStrWChar* picture);
    virtual void postMessage(const com::glu::platform::components::CStrWChar* text,
                             const com::glu::platform::components::CStrWChar* targetId,
                             const com::glu::platform::components::CStrWChar* link,
                             const com::glu::platform::components::CStrWChar* name,
                             const com::glu::platform::components::CStrWChar* caption,
                             const com::glu::platform::components::CStrWChar* description,
                             const com::glu::platform::components::CStrWChar* picture,
                             const com::glu::platform::components::CStrWChar* privacy);
    virtual unsigned char requestFriendPicWithID(const com::glu::platform::components::CStrWChar& id,
                                                 const com::glu::platform::components::CStrWChar* type);
    virtual unsigned char getFriendPicWithID(const com::glu::platform::components::CStrWChar& id, const void*& data, unsigned int& size);
    virtual void freeFriendPicWithID(const com::glu::platform::components::CStrWChar& id);
    virtual void freeAllFriendPics();

    /** The application was opened with `url` (Facebook login redirect). */
    BOOL handleOpenURL(NSURL* url);
};

#endif
