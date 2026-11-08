// CNGSDataOffer.cpp - gOffer offer records.
#include "CNGSDataOffer.h"

using com::glu::platform::components::CStrWChar;

CDataOffer::~CDataOffer()
{
}

void CInviteOffer::addPreviouslyInvitedFriend(int socialNetworkType, const CStrWChar& friendId)
{
    if (friendId.GetLength() != 0) {
        m_previouslyInvited[socialNetworkType].Add(friendId);
    }
}

int CDataOffer::hasFriendInviteIncentive()
{
    for (int index = 0; index < m_incentives.Size(); index++) {
        if (m_incentives[index].m_offerCategoryId == 1) {
            return index;
        }
    }
    return -1;
}

int CInviteOffer::getPotentialInvitee(CStrWChar* friendId)
{
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        CFriendInviteeList& invitees = m_potentialInvitees[network];
        if (invitees.Size() > 0 && invitees.m_list.m_head != NULL) {
            CStrWChar* first = (CStrWChar*)invitees.m_list.m_head->m_data;
            *friendId = *first;
            invitees.RemoveElement(*friendId);
            return network;
        }
    }
    return -1;
}
