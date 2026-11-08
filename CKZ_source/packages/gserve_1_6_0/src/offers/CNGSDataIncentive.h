// CNGSDataIncentive.h - one incentive (reward) granted by gOffer, as parsed from the
// "incentiveresult" array of the get-incentives reply (COfferManager::extractIncentivesFromResponse).
#ifndef CNGSDATAINCENTIVE_H
#define CNGSDATAINCENTIVE_H

#include "CStrWChar.h"

class CDataIncentive {
public:
    CDataIncentive();

    int m_quantity;                                                 // +0x00 "quantity"
    com::glu::platform::components::CStrWChar m_unitOfMeasurement; // +0x04 "unitOfMeasurement"
    int m_userGameRewardId;                                         // +0x14 "userGameRewardId"
    com::glu::platform::components::CStrWChar m_offeredGameName;    // +0x18 "offeredGameName"
    int m_incentiveCount;                                           // +0x28 "incentiveCount", 1 by default
    int m_offerCategoryId;                                          // +0x2c "offerCategoryId" (1: friend invite)
    com::glu::platform::components::CStrWChar m_offerCategoryName;  // +0x30 "offerCategoryName"
    com::glu::platform::components::CStrWChar m_invitedFriendName;  // +0x40 name of "invitedFriendIdentifier"
    int m_socialNetworkTypeId;                                      // +0x50 "socialNetworkTypeId" (not set by the constructor)
};

#endif
