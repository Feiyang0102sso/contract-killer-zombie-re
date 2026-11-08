// CNGSDataOffer.h - one gOffer offer and its incentives; CInviteOffer adds the per social
// network friend lists of the invite flow (already invited / still to invite).
// The meaning of the string fields is not visible in the offers code; their names are guesses.
#ifndef CNGSDATAOFFER_H
#define CNGSDATAOFFER_H

#include "CStrWChar.h"
#include "CVector.h"
#include "CNGSDataIncentive.h"
#include "CNGSFriendInviteeList.h"

class CDataOffer {
public:
    /** featureType values (m_featureType). */
    enum {
        FEATURE_TYPE_INVITE = 1,
        FEATURE_TYPE_FEATURED_GAME = 2
    };

    ~CDataOffer();

    /** Index of the first incentive of category 1 (friend invite), -1 when none. */
    int hasFriendInviteIncentive();

    com::glu::platform::components::CStrWChar m_name;          // +0x00
    com::glu::platform::components::CStrWChar m_description;   // +0x10
    int m_value20;                                              // +0x20 meaning unknown
    com::glu::platform::components::CStrWChar m_imageURL;      // +0x24
    int m_value34;                                              // +0x34 meaning unknown
    com::glu::platform::components::CStrWChar m_storeURL;      // +0x38
    int m_offerItemId;                                          // +0x48
    com::glu::platform::components::CStrWChar m_gameName;      // +0x4c
    int m_sourceGameId;                                         // +0x5c
    int m_featureType;                                          // +0x60 FEATURE_TYPE_*
    com::glu::platform::core::CVector<CDataIncentive> m_incentives;    // +0x64
};

class CInviteOffer : public CDataOffer {
public:
    /** Social networks with friend lists. */
    enum {
        SOCIAL_NETWORK_COUNT = 6
    };

    /** Remember that `friendId` of network `socialNetworkType` was invited (empty ids are ignored). */
    void addPreviouslyInvitedFriend(int socialNetworkType, const com::glu::platform::components::CStrWChar& friendId);
    /**
     * Take the first friend still to invite (lowest network first) into *friendId and drop it
     * from its list; returns its network, -1 when nobody is left.
     */
    int getPotentialInvitee(com::glu::platform::components::CStrWChar* friendId);

    CFriendInviteeList m_previouslyInvited[SOCIAL_NETWORK_COUNT];  // +0x7c
    CFriendInviteeList m_potentialInvitees[SOCIAL_NETWORK_COUNT];  // +0xf4
};

#endif
