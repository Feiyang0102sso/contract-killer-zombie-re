// PROVISIONAL (created by b077): owner src/bounty_hunter/CShopItem.cpp not written yet
// CShopItem.h - CItem (something sold in the shop) and CItemManager (a list of items read from a
// config). Virtual slots from the vtables in CShopItem.o / CGrenade.o / CIAPCurrency.o / CEnergyManager.o,
// fields from CItem::CItem, CItem::ParseOverridable and CItemManager::CItemManager.
// The "original" copies (m_orig*) keep the config values so an override can be undone.
// b074i: full owner recovery; the PROVISIONAL and old-name comments are kept as history, actual side effects follow the original instructions.
#ifndef CSHOPITEM_H
#define CSHOPITEM_H

#include <stddef.h>
#include "xString.h"
#include "DataProtection.h"

namespace com { namespace glu { namespace platform { namespace graphics {
class ICRenderSurface;
} } } }

class TiXmlNode;
class CZombieDialogWindow;
class CShopItemButton;

/** An item of the shop. No virtual destructor (the managers destroy their items directly). */
class CItem {
public:
    CItem();
    ~CItem()
    {
    }

    virtual void loadResources() = 0;                                       // 0x00
    virtual void releaseResources() = 0;                                    // 0x04
    virtual bool isHidden() const                                           // 0x08
    {
        return m_hidden;
    }
    virtual bool isAvailable() const                                        // 0x0c
    {
        return false;
    }
    virtual int getPriceCredits() const;                                    // 0x10
    virtual int getPricePoints() const;                                     // 0x14
    virtual bool IsEquiped() const                                          // 0x18
    {
        return false;
    }
    virtual bool IsOwned() const                                            // 0x1c
    {
        return false;
    }
    virtual void setAvailable(bool available) = 0;                          // 0x20
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage() = 0;     // 0x24
    virtual com::glu::platform::graphics::ICRenderSurface* GetSmallImage()  // 0x28
    {
        return GetShopImage();
    }
    virtual XString& GetName() = 0;                                         // 0x2c
    virtual XString& GetDescription() = 0;                                  // 0x30
    virtual int getUnlockLvl() const = 0;                                   // 0x34
    virtual bool IsHot() const                                              // 0x38
    {
        return m_isHot;
    }
    virtual bool IsNew() const                                              // 0x3c
    {
        return m_isNew;
    }
    virtual bool IsSale() const;                                            // 0x40
    virtual int GetSaleStartMS() const;                                     // 0x44
    virtual int GetSaleEndMS() const;                                       // 0x48
    virtual const char* GetItemTypeName() const = 0;                        // 0x4c
    virtual int getId() const = 0;                                          // 0x50
    virtual int GetCount() const                                            // 0x54
    {
        return 0;
    }
    /** 1 when bought. */
    virtual int Buy();                                                      // 0x58
    virtual bool CheckBuyConditions(CZombieDialogWindow** dialog)           // 0x5c
    {
        *dialog = NULL;
        return true;
    }
    virtual bool IsLocked() const;                                          // 0x60
    virtual bool CheckFilter(int filter);                                   // 0x64
    virtual void SetDailyDealParams(int discount, unsigned int startMS, unsigned int endMS);   // 0x68
    virtual int GetOriginalPriceCredits() const;                            // 0x6c
    virtual int GetOriginalPricePoints() const;                             // 0x70

    /** Read the overridable attributes (price, sale, flags); `keepAsOriginal` also stores the m_orig* copies. */
    void ParseOverridable(TiXmlNode* node, bool keepAsOriginal);
    void Override(TiXmlNode* node);
    XString& GetStringUId();  // changed by b055x: returns the cached m_stringUId (no copy in CDH_Weapon::Buy)
    bool IsOnSale() const;
    bool IsSaleWithTimer() const;
    int GetSaleTimeLeft() const;

    bool m_hidden;                          // 0x04
    bool m_origHidden;                      // 0x05
    Encrypted<int> m_priceCredits;          // 0x08
    Encrypted<int> m_origPriceCredits;      // 0x10
    Encrypted<int> m_pricePoints;           // 0x18
    Encrypted<int> m_origPricePoints;       // 0x20
    bool m_isHot;                           // 0x28
    bool m_origIsHot;                       // 0x29
    bool m_isNew;                           // 0x2a
    bool m_origIsNew;                       // 0x2b
    bool m_isSale;                          // 0x2c
    bool m_origIsSale;                      // 0x2d
    int m_saleStartMS;                      // 0x30
    int m_origSaleStartMS;                  // 0x34
    int m_saleEndMS;                        // 0x38
    int m_origSaleEndMS;                    // 0x3c
    Encrypted<int> m_salePriceCredits;      // 0x40
    Encrypted<int> m_origSalePriceCredits;  // 0x48
    Encrypted<int> m_salePricePoints;       // 0x50
    Encrypted<int> m_origSalePricePoints;   // 0x58
    // The historical field names swapped Credits and Points; original 120bdc/120bf6 and the getters prove the real mapping.
    Encrypted<int> m_dailyDealPoints;       // 0x60 (SetDailyDealParams)
    Encrypted<int> m_dailyDealCredits;      // 0x68
    unsigned int m_dailyDealStartMS;        // 0x70
    unsigned int m_dailyDealEndMS;          // 0x74
    bool m_isDailyDeal;                     // 0x78
    XString m_stringUId;                    // 0x7c cached by GetStringUId
};

/** Base of the managers of shop items (no virtual destructor). */
class CItemManager {
public:
    CItemManager();

    virtual int GetItemsCount() = 0;                                        // 0x00
    virtual int getAmount(int id)                                           // 0x04
    {
        return 0;
    }
    virtual CItem* getItemByName(XString name);                             // 0x08
    virtual CItem* getItemByStringUId(const XString& id);                   // 0x0c
    virtual CItem* getItemById(int id);                                     // 0x10
    virtual CShopItemButton* CreateShopItemButton(int index) = 0;           // 0x14
    virtual CItem* firstItem();                                             // 0x18
    virtual CItem* nextItem();                                              // 0x1c
    virtual void loadResources();                                           // 0x20
    virtual void releaseResources();                                        // 0x24
    virtual CItem* getItemByInternalId(int index) = 0;                      // 0x28
    virtual void AddItem(TiXmlNode* node) = 0;                              // 0x2c

    /** Has the config changed since it was loaded (CRC)? */
    bool IsConfigNew(const char* path);
    /** Load `path` and call AddItem for every `itemTag` under `rootTag` (NULL: the document). */
    void LoadFromConfig(const char* path, const char* rootTag, const char* itemTag);
    void CheckOverride();

    int m_iterator;                 // 0x04 firstItem / nextItem position
    int m_overrideVersion;          // 0x08 (CheckOverride)
    unsigned int m_configCrc;       // 0x0c
};

#endif
