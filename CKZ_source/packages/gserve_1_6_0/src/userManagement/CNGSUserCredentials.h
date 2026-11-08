// Owner: userManagement/CNGSUserCredentials.cpp (b030x; started as a b016x PROVISIONAL)
// CNGSUserCredentials.h - identity of a gServe user: client id, avatar id, nickname, email and
// one id + alias per social network (two new[6] arrays indexed by eSocialNetworkType).
// Persisted as JSON in the application data folder (readFromFile / writeToFile).
#ifndef CNGSUSERCREDENTIALS_H
#define CNGSUSERCREDENTIALS_H

#include "CStrWChar.h"

class CObjectMapObject;  // added by b025x

/** Social networks of the per network ids (values from the callers in messaging). */  // added by b023x
enum eSocialNetworkType {  // added by b023x
    SOCIAL_NETWORK_GAME_CENTER = 1,  // added by b023x
    SOCIAL_NETWORK_FACEBOOK = 2,  // added by b023x
    SOCIAL_NETWORK_TWITTER = 3,  // added by b023x
    SOCIAL_NETWORK_MYSPACE = 4,  // added by b023x
    SOCIAL_NETWORK_OPENFEINT = 5  // added by b023x
};  // added by b023x

/** Size of the per network id / alias arrays (index 0 is the Glu social network). */
static const int SOCIAL_NETWORK_COUNT = 6;

class CNGSUserCredentials {
public:
    CNGSUserCredentials();
    /** Credentials described by `object` (see InitFromObjectRepresentation). */
    CNGSUserCredentials(CObjectMapObject* object, bool fromServer);
    ~CNGSUserCredentials();

    /**
     * Read the fields from a JSON object: the credentials file layout ("clientid", "nickname",
     * per network name tables) or, with `fromServer`, a server user ("id", "gamertag",
     * "socialnetworkname" + "socialnetworkid"). 1 when a valid id and a nickname were found.
     */
    unsigned char InitFromObjectRepresentation(CObjectMapObject* object, bool fromServer);
    /** Back to "no user": ids -1, all strings empty. */
    void reset();

    CNGSUserCredentials& operator=(const CNGSUserCredentials& other);
    /** Load from the application data file `fileName`. */
    unsigned char readFromFile(com::glu::platform::components::CStrWChar& fileName, bool useKey);
    unsigned char writeToFile(com::glu::platform::components::CStrWChar& fileName, bool useKey);
    /** Take the known fields of `other` (all of them with `overwrite`); 0 when the client ids conflict. */
    unsigned char mergeWith(const CNGSUserCredentials& other, bool overwrite);
    unsigned char isValid() const;  // added by b023x
    unsigned char hasValidAvatar() const;
    com::glu::platform::components::CStrWChar getIDForSocialNetwork(eSocialNetworkType network) const;  // added by b023x
    void setIDForSocialNetwork(eSocialNetworkType network, const com::glu::platform::components::CStrWChar& id);
    com::glu::platform::components::CStrWChar getAliasForSocialNetwork(eSocialNetworkType network) const;
    void setAliasForSocialNetwork(eSocialNetworkType network, const com::glu::platform::components::CStrWChar& alias);
    /** JSON object of the credentials file layout (the caller owns it). */
    CObjectMapObject* CreateObjectRepresentation() const;
    /** Per user sub directory of the application data folder. */  // added by b023x
    com::glu::platform::components::CStrWChar getSubDirectoryPath() const;  // added by b023x
    /** Data folder + separator + sub directory of this user. */
    com::glu::platform::components::CStrWChar getFullDirectoryPath() const;
    static com::glu::platform::components::CStrWChar getFullDirectoryPathForSubdir(const com::glu::platform::components::CStrWChar& subDirectory);  // added by b023x
    /** Sub directory of user `userId` (static form). */  // added by b025x
    static com::glu::platform::components::CStrWChar getSubDirectoryPath(int userId);  // added by b025x
    /** {"id": ...} object of this user for a request (the caller owns it). */  // added by b025x
    CObjectMapObject* GetUserObject() const;  // added by b025x
    /** Avatar object of this user, NULL without avatar (the caller owns it). */  // added by b025x
    CObjectMapObject* GetAvatarObject() const;  // added by b025x
    /** Logs the credentials (debug builds). */  // added by b027x
    void debugPrint() const;  // added by b027x

private:
    /** Allocate the per network arrays, then reset(). */
    void alloc();

public:
    int m_userId;                                                   // +0x00 client id, -1 = none
    int m_avatarId;                                                 // +0x04 -1 = none
    com::glu::platform::components::CStrWChar m_nickName;           // +0x08
    com::glu::platform::components::CStrWChar m_email;              // +0x18
    com::glu::platform::components::CStrWChar* m_socialIds;        // +0x28 new[6]
    com::glu::platform::components::CStrWChar* m_socialAliases;    // +0x2c new[6]
};

#endif
