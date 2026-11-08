// CNGSOfferRequest.h - "content" payloads of the gOffer requests (COfferManager::sendRequest).
// Each class builds its JSON object with generateJSON(); the caller owns the result.
#ifndef CNGSOFFERREQUEST_H
#define CNGSOFFERREQUEST_H

#include "CStrWChar.h"
#include "CVector.h"
#include "CNGSFriendInviteeList.h"

class CObjectMapObject;

/** Base of the request payloads (no virtual destructor in the original). */
class COfferDataBase {
public:
    virtual CObjectMapObject* generateJSON() const = 0;

    /** Add "senderIdentifier" (the local user's client id as text) to `object`. */
    void generateBaseJSON(CObjectMapObject& object) const;
};

/** Offer selected: featureType, offerItemId, sourceGameId. */
class COfferDataSetSelected : public COfferDataBase {
public:
    COfferDataSetSelected();
    ~COfferDataSetSelected();

    virtual CObjectMapObject* generateJSON() const;

    int m_offerItemId;      // +0x04 -1 by default
    int m_sourceGameId;     // +0x08 -1 by default
    int m_featureType;      // +0x0c 1: invite offer, 2: featured game
};

/** Invite offer selected: also the invited friends of every social network. */
class CInviteOfferDataSetSelected : public COfferDataSetSelected {
public:
    /** Social networks with an invitee list. */
    enum {
        SOCIAL_NETWORK_COUNT = 6
    };

    CInviteOfferDataSetSelected();

    virtual CObjectMapObject* generateJSON() const;

    void resetInvitees();
    void addInvitee(int socialNetworkType, com::glu::platform::components::CStrWChar friendId);

    CFriendInviteeList m_invitees[SOCIAL_NETWORK_COUNT];   // +0x10 by social network type
};

/** Unconsumed incentives query. */
class COfferDataGetIncentives : public COfferDataBase {
public:
    COfferDataGetIncentives();
    ~COfferDataGetIncentives();

    virtual CObjectMapObject* generateJSON() const;

    int m_socialNetworkTypeId;      // +0x04 -1 by default
};

/** Incentives consumed: the user game reward ids. */
class COfferDataSetRewarded : public COfferDataBase {
public:
    COfferDataSetRewarded();
    ~COfferDataSetRewarded();

    virtual CObjectMapObject* generateJSON() const;

    com::glu::platform::core::CVector<unsigned int> m_userGameRewardIds;   // +0x04
};

/** Offer purchased: offerCategoryId, offerItemId. */
class COfferDataSetPurchased : public COfferDataBase {
public:
    COfferDataSetPurchased();

    virtual CObjectMapObject* generateJSON() const;

    int m_offerItemId;          // +0x04 (not set by the constructor)
    int m_offerCategoryId;      // +0x08 -1 by default
};

/** Install offer purchased: also the social network and the user id on it. */
class COfferDataSetPurchasedInstall : public COfferDataSetPurchased {
public:
    COfferDataSetPurchasedInstall();

    virtual CObjectMapObject* generateJSON() const;

    int m_socialNetworkTypeId;                                          // +0x0c -1 by default
    com::glu::platform::components::CStrWChar m_socialNetworkUserId;    // +0x10
};

#endif
