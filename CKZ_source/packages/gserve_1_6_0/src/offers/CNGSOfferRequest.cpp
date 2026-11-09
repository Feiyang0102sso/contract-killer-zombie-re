// CNGSOfferRequest.cpp - JSON payloads of the gOffer requests.
#include "CNGSOfferRequest.h"
#include "CObjectMapValue.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"
#include "ICStdUtil.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CLinkListNode;
using com::glu::platform::core::ICStdUtil;

void COfferDataBase::generateBaseJSON(CObjectMapObject& object) const
{
    CNGSUser* user = CNGS::GetInstance()->GetLocalUser();
    char clientId[256] = {0};
    ICStdUtil::SPrintF(clientId, "%i", user->GetClientID());
    object.addEntry(CStrWChar(L"senderIdentifier"), new CObjectMapString(CStrWChar(clientId)));
}

COfferDataSetSelected::COfferDataSetSelected()
{
    m_offerItemId = -1;
    m_sourceGameId = -1;
}

COfferDataSetSelected::~COfferDataSetSelected()
{
}

CObjectMapObject* COfferDataSetSelected::generateJSON() const
{
    CObjectMapObject* object = new CObjectMapObject();
    generateBaseJSON(*object);
    object->addEntry(CStrWChar(L"featureType"), new CObjectMapInt(m_featureType));
    object->addEntry(CStrWChar(L"offerItemId"), new CObjectMapInt(m_offerItemId));
    object->addEntry(CStrWChar(L"sourceGameId"), new CObjectMapInt(m_sourceGameId));
    return object;
}

CInviteOfferDataSetSelected::CInviteOfferDataSetSelected()
{
}

CObjectMapObject* CInviteOfferDataSetSelected::generateJSON() const
{
    CObjectMapObject* object = COfferDataSetSelected::generateJSON();
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        // Original quirk: the array leaks when the network has no invitee.
        CObjectMapArray* friendIds = new CObjectMapArray();
        int count = 0;
        // b147: the list accessors and addEntry are inline calls in the original (their order sets the stack slots)
        for (CLinkListNode* node = m_invitees[network].m_list.GetHead(); node != NULL; node = node->GetNext()) {
            CStrWChar* friendId = (CStrWChar*)node->GetData();
            if (friendId != NULL) {
                count++;
                friendIds->addEntry(new CObjectMapString(*friendId));
            }
        }
        if (count > 0) {
            CObjectMapObject* networkEntry = new CObjectMapObject();
            networkEntry->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(network));
            networkEntry->addEntry(CStrWChar(L"friendIds"), friendIds);
            object->addEntry(CStrWChar(L"friendsPerSocialNetworkType"), networkEntry);
        }
    }
    return object;
}

void CInviteOfferDataSetSelected::resetInvitees()
{
    for (int network = 0; network < SOCIAL_NETWORK_COUNT; network++) {
        m_invitees[network].Clear();
    }
}

void CInviteOfferDataSetSelected::addInvitee(int socialNetworkType, CStrWChar friendId)
{
    CStrWChar invitee = friendId;
    m_invitees[socialNetworkType].Add(invitee);
}

COfferDataGetIncentives::COfferDataGetIncentives()
{
    m_socialNetworkTypeId = -1;
}

COfferDataGetIncentives::~COfferDataGetIncentives()
{
    m_socialNetworkTypeId = -1;
}

CObjectMapObject* COfferDataGetIncentives::generateJSON() const
{
    CObjectMapObject* object = new CObjectMapObject();
    generateBaseJSON(*object);
    object->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(m_socialNetworkTypeId));
    return object;
}

COfferDataSetRewarded::COfferDataSetRewarded()
{
}

COfferDataSetRewarded::~COfferDataSetRewarded()
{
}

CObjectMapObject* COfferDataSetRewarded::generateJSON() const
{
    CObjectMapObject* object = new CObjectMapObject();
    generateBaseJSON(*object);
    CObjectMapArray* rewardIds = new CObjectMapArray();
    char text[256] = {0};
    // b147: values go through the inline CObjectMapArray::addEntry (its parameter slot is in the original frame)
    for (unsigned int index = 0; index < (unsigned int)m_userGameRewardIds.Size(); index++) {
        ICStdUtil::SPrintF(text, "%i", m_userGameRewardIds[index]);
        rewardIds->addEntry(new CObjectMapString(CStrWChar(text)));
    }
    object->addEntry(CStrWChar(L"userGameRewardIds"), rewardIds);
    return object;
}

COfferDataSetPurchased::COfferDataSetPurchased()
{
    m_offerCategoryId = -1;
}

CObjectMapObject* COfferDataSetPurchased::generateJSON() const
{
    CObjectMapObject* object = new CObjectMapObject();
    generateBaseJSON(*object);
    object->addEntry(CStrWChar(L"offerCategoryId"), new CObjectMapInt(m_offerCategoryId));
    object->addEntry(CStrWChar(L"offerItemId"), new CObjectMapInt(m_offerItemId));
    return object;
}

COfferDataSetPurchasedInstall::COfferDataSetPurchasedInstall()
{
    m_socialNetworkTypeId = -1;
    m_socialNetworkUserId = L"";
}

CObjectMapObject* COfferDataSetPurchasedInstall::generateJSON() const
{
    CObjectMapObject* object = COfferDataSetPurchased::generateJSON();
    object->addEntry(CStrWChar(L"socialNetworkTypeId"), new CObjectMapInt(m_socialNetworkTypeId));
    object->addEntry(CStrWChar(L"socialNetworkUserId"), new CObjectMapString(m_socialNetworkUserId));
    return object;
}
