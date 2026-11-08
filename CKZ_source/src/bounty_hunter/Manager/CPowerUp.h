// PROVISIONAL (created by b055x): owner src/bounty_hunter/Manager/CPowerUp.cpp not written yet
// CPowerUp.h - a power up of the shop; only the multipliers read by src/game/CDH_Weapon.cpp are named.
// Restored by b078a: 0xd0 bytes; offsets and virtual slots checked against CPowerUp.o.
#ifndef CPOWERUP_H
#define CPOWERUP_H

#include "CShopItem.h"

class CPowerUp : public CItem {
public:
    CPowerUp(TiXmlNode* node);
    ~CPowerUp();

    virtual void loadResources();
    virtual void releaseResources();
    virtual void setAvailable(bool available);
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage();
    virtual com::glu::platform::graphics::ICRenderSurface* GetSmallImage()
    {
        return GetShopImage();
    }
    virtual XString& GetName() { return m_name; }
    virtual XString& GetDescription() { return m_description; }
    virtual int getUnlockLvl() const { return 1; }
    virtual const char* GetItemTypeName() const { return "powerup"; }
    virtual int getId() const { return m_id; }
    virtual int GetCount() const { return m_count.Get(); }
    virtual int Buy();
    virtual bool CheckBuyConditions(CZombieDialogWindow** dialog);

    void RunVisualEffect();
    void StopVisualEffect();

    // Former m_data80[0x90 - 0x80]: layout not known yet (b055x).
    int m_id;                              // 0x80
    XString m_description;                 // 0x84
    XString m_name;                        // 0x88
    int m_effectTime;                      // 0x8c milliseconds
    float m_damageMultiplier;               // 0x90 (CDH_Weapon::GetShotDamage)
    float m_reloadSpeed;                    // 0x94 reload animation speed (CDH_Weapon::PlayReloadAnimation)
    float m_fireSpeed;                      // 0x98 shoot / throw animation speed (CDH_Weapon::ModifyDurations)
    float m_switchSpeed;                    // 0x9c fetch / hide animation speed (CDH_Weapon::ModifyDurations)
    float m_reloadSoundSpeed;               // 0xa0 not 1: the reload sounds are muted (CDH_Weapon::playReloadSound)
    // These strings are constructed and destroyed, but have no surviving accessor.
    XString m_fieldA4;                      // 0xa4 meaning unresolved
    XString m_fieldA8;                      // 0xa8 meaning unresolved
    XString m_fieldAc;                      // 0xac meaning unresolved
    bool m_resourcesLoaded;                 // 0xb0
    const char* m_shopImageResource;         // 0xb4
    com::glu::platform::graphics::ICRenderSurface* m_shopImage; // 0xb8
    const char* m_hudImageResource;          // 0xbc
    int m_blurEffect;                       // 0xc0 0 none, 1 in, 2 out
    int m_effectImage;                      // 0xc4 HUD effect id, -1 none
    Encrypted<int> m_count;                 // 0xc8
};

#endif
