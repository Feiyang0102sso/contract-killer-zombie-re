// CShopPack.h - shop bundles and the inventory item each bundle replenishes.
#ifndef CSHOPPACK_H
#define CSHOPPACK_H

#include "CShopItem.h"
#include "CGrenade.h"

class CPowerUp;
class ImageWindow;

/** Common bundle data; the derived type owns the inventory operation. */
class CShopPack : public CItem {
public:
    CShopPack(TiXmlNode* node);
    ~CShopPack();

    void ParseNode(TiXmlNode* node);
    virtual void loadResources();
    virtual void releaseResources();
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage();
    virtual com::glu::platform::graphics::ICRenderSurface* GetSmallImage()
    {
        return GetShopImage();
    }
    virtual XString& GetName() { return m_name; }
    virtual XString& GetDescription() { return m_description; }
    virtual int getUnlockLvl() const { return m_unlockLevel; }
    virtual int getId() const { return m_id; }
    virtual int Buy();
    virtual bool CheckBuyConditions(CZombieDialogWindow** dialog);
    virtual int GetInPackCount() const { return m_packCount; }  // 0x74

    int m_packCount;                       // 0x80
    int m_id;                              // 0x84
    int m_unlockLevel;                     // 0x88
    bool m_resourcesLoaded;                // 0x8c
    XString m_packImageResource;            // 0x90
    com::glu::platform::graphics::ICRenderSurface* m_packImage;  // 0x94
    const char* m_shopImageResource;        // 0x98
    com::glu::platform::graphics::ICRenderSurface* m_shopImage;  // 0x9c
    XString m_description;                 // 0xa0
    XString m_name;                        // 0xa4
};

class CGrenadePack : public CShopPack {
public:
    CGrenadePack(TiXmlNode* node);
    virtual void setAvailable(bool available);
    virtual bool CheckFilter(int filter);
    virtual bool IsEquiped() const;
    virtual bool IsOwned() const;
    ImageWindow* CreatePackImage();
    virtual const char* GetItemTypeName() const { return "grenade_pack"; }
    virtual int GetCount() const { return m_grenade->GetCount(); }

    CGrenade* m_grenade;                   // 0xa8
};

class CPowerUpPack : public CShopPack {
public:
    CPowerUpPack(TiXmlNode* node);
    virtual void setAvailable(bool available);
    virtual bool CheckFilter(int filter);
    virtual bool IsEquiped() const;
    virtual bool IsOwned() const;
    virtual int GetCount() const;
    virtual const char* GetItemTypeName() const { return "powerup_pack"; }

    CPowerUp* m_powerUp;                   // 0xa8
};

#endif
