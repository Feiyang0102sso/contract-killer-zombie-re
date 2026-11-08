// CNGSSKUBonus.cpp - SKU bonus of the local user.
// The original object also uses CAttributeManager in code the linker dead-stripped (its
// __GLOBAL__I_a stores ClassId_CAttributeManager); that code is not known, so it is not here.
#include "CNGSSKUBonus.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "BundleInterface.h"
#include "CVector.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;

/** Bonus message id passed to CNGSLocalUser::HandleBonusMessage (meaning not known). */
static const long long SKU_BONUS_MESSAGE_ID = 0x33;

CNGSSKUBonus::CNGSSKUBonus()
    : m_awardInGame(0)
{
    m_unused18 = 0;
}

void CNGSSKUBonus::AwardSKUBonusInGame(bool inGame)
{
    CNGSLocalUser* user = CNGS::GetInstance()->GetLocalUser();
    m_awardInGame = inGame;
    CStrWChar message(L"default bonus message");
    CVector<CNGSAttribute*> items;
    if (m_awardInGame) {
        CStrWChar bundleIdentifier;
        BundleInterface::BundleIdentifierString(bundleIdentifier);
        user->GetSKUBonusItems(bundleIdentifier, items);
        if (items.Size() == 0) {
            m_awardInGame = 0;
        }
    }
    user->HandleBonusMessage(m_awardInGame, SKU_BONUS_MESSAGE_ID, message, items, user->GetClientID());
}
