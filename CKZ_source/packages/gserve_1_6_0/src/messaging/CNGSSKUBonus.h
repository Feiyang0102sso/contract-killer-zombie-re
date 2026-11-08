// CNGSSKUBonus.h - bonus items granted for owning a SKU (the bundle identifier of the app):
// the local user looks the items up and reports them as a bonus message (b023x).
#ifndef CNGSSKUBONUS_H
#define CNGSSKUBONUS_H

#include "CNGSServerObject.h"
#include "CNGSUserCredentials.h"
#include "CNGSAttributeManager.h"

class CNGSSKUBonus : public CNGSServerObject {
public:
    CNGSSKUBonus();

    virtual ~CNGSSKUBonus() {}

    /** Hand the SKU bonus items of this bundle to the local user (`inGame`: award them now). */
    virtual void AwardSKUBonusInGame(bool inGame);

    int m_unused18;                         // +0x18 cleared by the constructor, not used here
    bool m_awardInGame;                     // +0x1c cleared when the SKU has no bonus items
    CNGSUserCredentials m_credentials;      // +0x20
};

#endif
