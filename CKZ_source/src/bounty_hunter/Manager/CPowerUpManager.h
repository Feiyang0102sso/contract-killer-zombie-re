// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/CPowerUpManager.cpp not written yet
// CPowerUpManager.h - power ups; only what src/general/App.cpp uses is declared.
// Restored by b078a: CItemManager base, vector and active-power-up state (0x28 bytes).
#ifndef CPOWERUPMANAGER_H
#define CPOWERUPMANAGER_H

#include <stddef.h>
#include "xString.h"
#include "CShopItem.h"
#include "Vector_ss.h"

class CShopItem;
class CPowerUp;  // added by b055x

class TiXmlElement;

class CPowerUpManager : public CItemManager {
public:
    CPowerUpManager();
    ~CPowerUpManager();
    virtual int GetItemsCount() { return m_items.size(); }
    virtual CShopItemButton* CreateShopItemButton(int index);
    virtual CItem* getItemByInternalId(int index);
    virtual void AddItem(TiXmlNode* node);
    /** The item with the string id (NULL: not managed here). */
    CItem* GetItemByStringUId(const XString& id) { return getItemByStringUId(id); }

    void Save(TiXmlElement* parent);
    void Load(TiXmlElement* parent);
    void SetActivePowerUp(int id);
    void Update(int elapsedMs);
    void FinishRunningPowerUp();
    bool IsPowerUpAvailable();
    float GetPowerUpLeftPercent();
    void RunPowerUp();

    /** True while a power up is active. */  // added by b055x
    bool IsPowerUpRunning();  // added by b055x
    /** The active power up (its multipliers are read even when none runs). */  // added by b055x
    CPowerUp* GetActivePowerUp();  // added by b055x

    // Former m_data04[0x28 - 0x4]: layout not known yet (size from App.cpp).
    Vector<CPowerUp*> m_items;              // 0x10
    int m_activeId;                         // 0x20 -1 means none selected
    int m_timeLeft;                         // 0x24 milliseconds
};

#endif
