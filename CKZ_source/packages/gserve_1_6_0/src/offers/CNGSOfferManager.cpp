// CNGSOfferManager.cpp - gOffer client: offer selection / purchase and unconsumed incentives.
#include "CNGSOfferManager.h"
#include "CNGSURLMgr.h"
#include "CNGSRequestFunctor.h"
#include "CNGSOfferDelegate.h"
#include "CNGSHeader.h"
#include "CNGSServerRequest.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGS_Platform.h"
#include "CNGSSocialInterface.h"
#include "CNotificationHandler.h"
#include "CObjectMap.h"
#include "CObjectMapValue.h"
#include "CObjectMapObject.h"
#include "CObjectMapArray.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CStrChar;
using com::glu::platform::core::CVector;

/** Error code reported to the notification handler when a reply could not be parsed. */
static const int OFFER_ERROR_BAD_REPLY = 11;

typedef CNGSServerRequestCompleteFunctor<COfferManager> COfferManagerFunctor;

COfferManager::COfferManager()
    : CSingleton(ClassId_COfferManager)
{
    m_notificationHandler = NULL;
    m_offersLoaded = 0;
    m_state = STATE_IDLE;
    m_selectedOfferIndex = 0;
    m_inviteDelegate = new InviteDelegate();
    m_featuredGameDelegate = new FeaturedGameDelegate();
}

COfferManager::~COfferManager()
{
    resetOffers();
    resetUnconsumedIncentives();
    for (int index = 0; index < m_offers.Size(); index++) {
        if (m_offers[index] != NULL) {
            delete m_offers[index];
            m_offers[index] = NULL;
        }
    }
    if (m_inviteDelegate != NULL) {
        delete m_inviteDelegate;
        m_inviteDelegate = NULL;
    }
    if (m_featuredGameDelegate != NULL) {
        delete m_featuredGameDelegate;
        m_featuredGameDelegate = NULL;
    }
}

unsigned char COfferManager::resetOffers()
{
    for (int index = 0; index < m_offers.Size(); index++) {
        if (m_offers[index] != NULL) {
            delete m_offers[index];
            m_offers[index] = NULL;
        }
    }
    m_offers.Clear();
    m_state = STATE_IDLE;
    m_offersLoaded = 0;
    return 1;
}

void COfferManager::resetUnconsumedIncentives()
{
    for (int index = 0; index < m_unconsumedIncentives.Size(); index++) {
        if (m_unconsumedIncentives[index] != NULL) {
            delete m_unconsumedIncentives[index];
            m_unconsumedIncentives[index] = NULL;
        }
    }
    m_unconsumedIncentives.Clear();
    m_rewardedData.m_userGameRewardIds.Clear();
}

unsigned char COfferManager::selectOffer(int index)
{
    m_state = STATE_SELECTING;
    CDataOffer* offer = m_offers[index];
    switch (offer->m_featureType) {
    case CDataOffer::FEATURE_TYPE_INVITE:
        m_inviteDelegate->m_selectedData->m_offerItemId = offer->m_offerItemId;
        m_inviteDelegate->m_selectedData->m_sourceGameId = offer->m_sourceGameId;
        m_inviteDelegate->m_selectedData->m_featureType = CDataOffer::FEATURE_TYPE_INVITE;
        sendRequest(SERVICE_OFFER_SELECTED, m_inviteDelegate->m_selectedData,
                    new COfferManagerFunctor(this, &COfferManager::handleResponse_select));
        break;
    case CDataOffer::FEATURE_TYPE_FEATURED_GAME:
        m_featuredGameDelegate->m_selectedData->m_offerItemId = offer->m_offerItemId;
        m_featuredGameDelegate->m_selectedData->m_sourceGameId = offer->m_sourceGameId;
        m_featuredGameDelegate->m_selectedData->m_featureType = CDataOffer::FEATURE_TYPE_FEATURED_GAME;
        sendRequest(SERVICE_OFFER_SELECTED, m_featuredGameDelegate->m_selectedData,
                    new COfferManagerFunctor(this, &COfferManager::handleResponse_select));
        break;
    }
    return 1;
}

void COfferManager::handleResponse_select(CObjectMap* response, CNGSServerRequestFunctor* functor)
{
    COffer offer;
    unsigned char parsed = parseResponse(response, offer);
    switch (offer.m_featureType) {
    case CDataOffer::FEATURE_TYPE_INVITE:
        if (parsed == 1) {
            // Invite offers with a friend invite incentive go on to the purchase.
            if (m_offers.Size() > m_selectedOfferIndex
                && m_offers[m_selectedOfferIndex]->hasFriendInviteIncentive() != -1) {
                setPurchasedOffer(1, offer.m_offerItemId);
            } else {
                m_state = STATE_READY;
                m_notificationHandler->HandleSendInvite(parsed, m_invitesSent);
            }
        } else {
            m_state = STATE_READY;
            m_notificationHandler->HandleSendInvite(parsed, OFFER_ERROR_BAD_REPLY);
        }
        break;
    case CDataOffer::FEATURE_TYPE_FEATURED_GAME:
        if (parsed == 1) {
            setPurchasedOffer(3, offer.m_offerItemId);
        } else {
            m_state = STATE_READY;
            m_notificationHandler->HandleFeaturedGameSelected(parsed, OFFER_ERROR_BAD_REPLY);
        }
        break;
    }
}

unsigned char COfferManager::parseResponse(CObjectMap* response, COffer& offer)
{
    unsigned char parsed = 0;
    if (response == NULL) {
        return parsed;
    }
    CObjectMapObject* content = (CObjectMapObject*)response->getDataAt(CStrWChar(L"content"), CStrWChar(L" "));
    if (content == NULL) {
        return parsed;
    }
    CObjectMapBool* success = (CObjectMapBool*)content->getEntry(CStrWChar(L"success"));
    if (success->m_value) {
        CObjectMapInt* featureType = (CObjectMapInt*)content->getEntry(CStrWChar(L"featureType"));
        CObjectMapInt* offerItemId = (CObjectMapInt*)content->getEntry(CStrWChar(L"offerItemId"));
        CObjectMapInt* offerCategoryId = (CObjectMapInt*)content->getEntry(CStrWChar(L"offerCategoryId"));
        if (featureType != NULL) {
            offer.m_featureType = (int)featureType->m_value;
        }
        if (offerItemId != NULL) {
            offer.m_offerItemId = (int)offerItemId->m_value;
        }
        parsed = 1;
        if (offerCategoryId != NULL) {
            offer.m_offerCategoryId = (int)offerCategoryId->m_value;
            parsed = 1;
        }
    }
    return parsed;
}

void COfferManager::setPurchasedOffer(int offerCategoryId, int offerItemId)
{
    m_state = STATE_PURCHASING;
    COfferDataSetPurchased* data;
    if (offerCategoryId == 2) {
        data = new COfferDataSetPurchasedInstall();
    } else {
        data = new COfferDataSetPurchased();
    }
    data->m_offerItemId = offerItemId;
    data->m_offerCategoryId = offerCategoryId;
    // Original quirk: the payload is never deleted.
    sendRequest(SERVICE_OFFER_PURCHASED, data, new COfferManagerFunctor(this, &COfferManager::handleResponse_purchase));
}

void COfferManager::finishedExecuting(int type, int status, int count)
{
    if (m_state != STATE_EXECUTING) {
        return;
    }
    switch (type) {
    case CDataOffer::FEATURE_TYPE_INVITE:
        if (status == 0) {
            m_invitesSent = count;
            selectOffer(m_selectedOfferIndex);
        } else {
            m_notificationHandler->HandleSendInvite(0, status);
            m_state = STATE_READY;
        }
        break;
    case CDataOffer::FEATURE_TYPE_FEATURED_GAME:
        if (status == 0) {
            selectOffer(m_selectedOfferIndex);
        } else {
            m_notificationHandler->HandleFeaturedGameSelected(0, status);
            m_state = STATE_READY;
        }
        break;
    }
}

void COfferManager::handlePostMessage(unsigned char success)
{
    m_inviteDelegate->handlePostMessage(success);
}

void COfferManager::handleResponse_purchase(CObjectMap* response, CNGSServerRequestFunctor* functor)
{
    m_state = STATE_READY;
    COffer offer;
    unsigned char parsed = parseResponse(response, offer);
    switch (offer.m_offerCategoryId) {
    case 1: {
        int invites = OFFER_ERROR_BAD_REPLY;
        if (parsed == 1) {
            invites = m_invitesSent;
        }
        m_state = STATE_READY;
        m_notificationHandler->HandleSendInvite(parsed, invites);
        break;
    }
    case 3: {
        m_state = STATE_READY;
        // ?: gives the original select (default 11, it ne; movne 0)
        m_notificationHandler->HandleFeaturedGameSelected(parsed, parsed ? 0 : OFFER_ERROR_BAD_REPLY);
        break;
    }
    }
}

void COfferManager::setPurchasedOfferInstall(int socialNetworkType)
{
    CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(socialNetworkType);
    CStrWChar playerId = social->getPlayerID();
    COfferDataSetPurchasedInstall* data = new COfferDataSetPurchasedInstall();
    data->m_socialNetworkTypeId = socialNetworkType;
    data->m_socialNetworkUserId = playerId;
    data->m_offerCategoryId = 2;
    // Original quirk: the payload is never deleted.
    sendRequest(SERVICE_OFFER_PURCHASED, data, new COfferManagerFunctor(this, &COfferManager::handleResponse_purchase));
}

unsigned char COfferManager::getUnconsumedIncentives()
{
    if (m_state == STATE_READY && CNGS::GetInstance()->GetLocalUser()->isValid()) {
        m_state = STATE_GETTING_INCENTIVES;
        COfferDataGetIncentives data;
        sendRequest(SERVICE_INCENTIVE, &data, new COfferManagerFunctor(this, &COfferManager::handleResponse_getIncentive));
        return 1;
    }
    m_notificationHandler->HandleIncentive(0, 0);
    return 0;
}

void COfferManager::handleResponse_getIncentive(CObjectMap* response, CNGSServerRequestFunctor* functor)
{
    CStrWChar errorMessage(L"failed");
    if (CNGSHeader::checkResponseOk(response, errorMessage) == 1) {
        resetUnconsumedIncentives();
        extractIncentivesFromResponse(response, m_unconsumedIncentives);
        if (m_notificationHandler != NULL) {
            if (m_unconsumedIncentives.Size() > 0) {
                m_notificationHandler->HandleIncentive(1, m_unconsumedIncentives.Size());
            } else {
                m_notificationHandler->HandleIncentive(0, 0);
            }
        }
    } else if (m_notificationHandler != NULL) {
        m_notificationHandler->HandleIncentive(0, 0);
    }
    m_state = STATE_READY;
}

unsigned char COfferManager::extractIncentivesFromResponse(CObjectMap* response, CVector<CDataIncentive*>& incentives)
{
    unsigned char extracted = 0;
    if (response != NULL) {
        CObjectMapObject* content = (CObjectMapObject*)response->getDataAt(CStrWChar(L"content"), CStrWChar(L" "));
        if (content != NULL) {
            CObjectMapBool* success = (CObjectMapBool*)content->getEntry(CStrWChar(L"success"));
            if (success->m_value) {
                CObjectMapArray* results = (CObjectMapArray*)content->getEntry(CStrWChar(L"incentiveresult"));
                extracted = 1;
                if (results != NULL && results->getType() == CObjectMapValue::TYPE_ARRAY) {
                    for (int index = 0; index < results->m_values.Size(); index++) {
                        CObjectMapObject* result = (CObjectMapObject*)results->m_values[index];
                        CObjectMapInt* quantity = (CObjectMapInt*)result->getEntry(CStrWChar(L"quantity"));
                        CObjectMapString* unitOfMeasurement = (CObjectMapString*)result->getEntry(CStrWChar(L"unitOfMeasurement"));
                        CObjectMapInt* userGameRewardId = (CObjectMapInt*)result->getEntry(CStrWChar(L"userGameRewardId"));
                        CObjectMapString* offeredGameName = (CObjectMapString*)result->getEntry(CStrWChar(L"offeredGameName"));
                        CObjectMapInt* incentiveCount = (CObjectMapInt*)result->getEntry(CStrWChar(L"incentiveCount"));
                        CObjectMapString* invitedFriend = (CObjectMapString*)result->getEntry(CStrWChar(L"invitedFriendIdentifier"));
                        CObjectMapInt* offerCategoryId = (CObjectMapInt*)result->getEntry(CStrWChar(L"offerCategoryId"));
                        CObjectMapString* offerCategoryName = (CObjectMapString*)result->getEntry(CStrWChar(L"offerCategoryName"));
                        CObjectMapInt* socialNetworkTypeId = (CObjectMapInt*)result->getEntry(CStrWChar(L"socialNetworkTypeId"));

                        CDataIncentive* incentive = new CDataIncentive();
                        if (socialNetworkTypeId != NULL) {
                            incentive->m_socialNetworkTypeId = (int)socialNetworkTypeId->m_value;
                        }
                        // Only incentives of a network the user is logged in to are kept (the others leak).
                        CNGSSocialInterface* social = CNGS_Platform::GetInstance()->getSocialInterface(incentive->m_socialNetworkTypeId);
                        if (social->isUserAuthenticated()) {
                            if (quantity != NULL) {
                                incentive->m_quantity = (int)quantity->m_value;
                            }
                            if (unitOfMeasurement != NULL) {
                                incentive->m_unitOfMeasurement = CStrWChar(unitOfMeasurement->m_value);
                            }
                            if (userGameRewardId != NULL) {
                                incentive->m_userGameRewardId = (int)userGameRewardId->m_value;
                            }
                            if (offeredGameName != NULL) {
                                incentive->m_offeredGameName = CStrWChar(offeredGameName->m_value);
                            }
                            if (incentiveCount != NULL) {
                                incentive->m_incentiveCount = (int)incentiveCount->m_value;
                            }
                            if (invitedFriend != NULL) {
                                incentive->m_invitedFriendName = social->getFriendNameFromSortedListWithId(CStrWChar(invitedFriend->m_value));
                            }
                            if (offerCategoryId != NULL) {
                                incentive->m_offerCategoryId = (int)offerCategoryId->m_value;
                            }
                            if (offerCategoryName != NULL) {
                                incentive->m_offerCategoryName = CStrWChar(offerCategoryName->m_value);
                            }
                            incentives.Add(incentive);
                        }
                    }
                }
            }
        }
    }
    return extracted;
}

// Reconstructed by b028x: the shipped binary dead-stripped the code that fills m_rewardedData,
// but CNGSOfferManager.o still carries the CVector<unsigned int> instantiations it used
// (AddAll, constructor, destructors). Minimal form: collect the reward ids of the unconsumed incentives.
void COfferManager::collectRewardedIncentives()
{
    for (int index = 0; index < m_unconsumedIncentives.Size(); index++) {
        m_rewardedData.m_userGameRewardIds.Add(m_unconsumedIncentives[index]->m_userGameRewardId);
    }
}

unsigned int COfferManager::sendRequest(int service, const COfferDataBase* data, CNGSServerRequestFunctor* functor)
{
    CNGSHeader header;
    header.m_carrier = L"itunes";
    CObjectMapObject* message = new CObjectMapObject();
    message->addEntry(CStrWChar(L"header"), header.createObjectRepresentation(1));
    message->addEntry(CStrWChar(L"content"), data->generateJSON());
    CStrChar url = CNGSURLMgr::GetInstance()->getGOfferURL((eGOfferServiceType)service);
    CObjectMap* map = new CObjectMap(message);
    if (message != NULL) {
        delete message;
    }
    CNGSServerRequest* request = new CNGSServerRequest(url.GetCharPtr(), map, functor, 0, 1);
    return request->m_messageId;
}
