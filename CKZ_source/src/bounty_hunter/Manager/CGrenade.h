// CGrenade.h - a grenade type of grenades.xml: shop item, explosion and physics parameters.
// (b077 replaced the PROVISIONAL of b055x; m_count and m_throwTime keep their names.)
#ifndef CGRENADE_H
#define CGRENADE_H

#include "CShopItem.h"

class TiXmlNode;

class CGrenade : public CItem {
public:
    CGrenade(TiXmlNode* node);
    ~CGrenade();

    virtual void loadResources();                                           // 0x00
    virtual void releaseResources();                                        // 0x04
    virtual void setAvailable(bool available);                              // 0x20
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage();  // 0x24
    virtual XString& GetName()                                              // 0x2c
    {
        return m_name;
    }
    virtual XString& GetDescription()                                       // 0x30
    {
        return m_description;
    }
    virtual int getUnlockLvl() const                                        // 0x34
    {
        return 1;
    }
    virtual const char* GetItemTypeName() const                             // 0x4c
    {
        return "grenade";
    }
    virtual int getId() const                                               // 0x50
    {
        return m_id;
    }
    virtual int GetCount() const                                            // 0x54
    {
        return m_count.Get();
    }
    virtual int Buy();                                                      // 0x58
    virtual bool CheckBuyConditions(CZombieDialogWindow** dialog);          // 0x5c

    /** Grenades owned (kept encrypted). */
    void SetCount(int count)
    {
        m_count.Set(count);
    }

    int m_id;                               // 0x80
    XString m_description;                  // 0x84
    XString m_name;                         // 0x88
    Encrypted<int> m_count;                 // 0x8c grenades owned (CDH_Weapon::Throw)
    const char* m_model;                    // 0x94 m3g resource id
    int m_damageCenter;                     // 0x98
    int m_damagePeripheral;                 // 0x9c
    int m_explosionRadius;                  // 0xa0
    int m_delayAfterContact;                // 0xa4
    int m_throwTime;                        // 0xa8 "cooldown": minimum throw duration in ms (CDH_Weapon::Throw)
    float m_bounce;                         // 0xac clamped to [0, 1]
    float m_friction;                       // 0xb0 [0, 1]
    float m_drag;                           // 0xb4 [0, 1]
    float m_shardBounce;                    // 0xb8 [0, 1]
    float m_shardFriction;                  // 0xbc [0, 1]
    float m_shardDrag;                      // 0xc0 [0, 1]
    float m_shardCount;                     // 0xc4 >= 0
    float m_shardSpeed;                     // 0xc8 >= 0
    float m_shardMulti;                     // 0xcc >= 0
    float m_shardAngle;                     // 0xd0 >= 0
    bool m_isBeacon;                        // 0xd4 lure grenade
    XString m_damageDescription;            // 0xd8
    XString m_radiusDescription;            // 0xdc
    XString m_delayDescription;             // 0xe0
    bool m_resourcesLoaded;                 // 0xe4
    const char* m_shopImage;                // 0xe8 resource id
    com::glu::platform::graphics::ICRenderSurface* m_shopSurface;   // 0xec
    const char* m_hudImage;                 // 0xf0 resource id
};

#endif
