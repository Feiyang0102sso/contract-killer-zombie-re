// Owner: userManagement/CNGSUser.cpp (b030x; started as a b016x PROVISIONAL)
// CNGSUser.h - a gServe user (local or remote): server object plus credentials.
#ifndef CNGSUSER_H
#define CNGSUSER_H

#include "CNGSServerObject.h"
#include "CNGSUserCredentials.h"

class CObjectMapObject;  // added by b023x
class CNGSAttribute;  // added by b027x
#include "CVector.h"  // added by b027x

class CNGSUser : public CNGSServerObject {
public:
    /** Copies `credentials` when given. */
    CNGSUser(const CNGSUserCredentials* credentials);
    virtual ~CNGSUser();

    /** The user's data arrived from the server (never for a plain user). */
    virtual unsigned char IsPopulated() const
    {
        return 0;
    }

    /** Write the credentials to `fileName`: once in the data folder, once in the user's folder. */
    void SaveCredentials(const wchar_t* fileName);
    void LoadCredentials(const wchar_t* fileName);
    /** Forget the credentials; also delete `fileName` when given. */
    void ResetCredentials(const wchar_t* fileName);
    static void DeleteCredentialsFile(const wchar_t* fileName);

    int GetClientID() const;  // added by b023x
    /** {"id": ...} object describing this user for a request (the caller owns it). */  // added by b023x
    CObjectMapObject* GetUserObject();  // added by b023x

    // vtable slots +0x20..+0x28 (order from the original vtable)  // added by b027x
    virtual void GetSKUBonusItems(com::glu::platform::components::CStrWChar& sku, com::glu::platform::core::CVector<CNGSAttribute*>& items);  // added by b027x
    virtual com::glu::platform::components::CStrWChar GetSKUBonusNameForItem(CNGSAttribute* item);  // added by b027x
    /** Pure in the original (slot +0x28 is empty in the CNGSUser vtable). */
    virtual unsigned char IsLocalUser() = 0;  // added by b027x (made pure by b030x)
    void SetNickName(const com::glu::platform::components::CStrWChar& nickName);  // added by b027x
    com::glu::platform::components::CStrWChar GetNickName() const;  // added by b027x
    com::glu::platform::components::CStrWChar GetEmail() const;
    void SetIDForSocialNetwork(eSocialNetworkType network, const com::glu::platform::components::CStrWChar& id);  // added by b027x
    com::glu::platform::components::CStrWChar GetIDForSocialNetwork(eSocialNetworkType network) const;  // added by b027x
    com::glu::platform::components::CStrWChar GetAliasForSocialNetwork(eSocialNetworkType network) const;
    void SetAliasForSocialNetwork(eSocialNetworkType network, const com::glu::platform::components::CStrWChar& alias);  // added by b027x
    /** Store the last message time of friend `userId` (a known remote user). */
    void setFriendMsgTimestampLocally(int userId, unsigned long long timestamp);
    /** Merge `credentials` into ours; replace ours when the client ids conflict. */
    void UpdateCredentials(const CNGSUserCredentials& credentials);
    void DebugPrint();

    CNGSUserCredentials m_credentials;      // +0x18
};

#endif
