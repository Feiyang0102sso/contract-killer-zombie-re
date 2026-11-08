// PROVISIONAL (created by b077): owner src/bounty_hunter/Manager/CStatsBonusItem.cpp not written yet
// CStatsBonusItem.h - an energy / stats bonus item of the shop; only what CEnergyManager uses.
// Overrides from the CStatsBonusItem vtable; the inline ones (weak in CStatsBonusItem.o) are only
// declared here until the owner writes them.
#ifndef CSTATSBONUSITEM_H
#define CSTATSBONUSITEM_H

#include "CShopItem.h"
#include "PlayerStats.h"

class TiXmlNode;

class CStatsBonusItem : public CItem {
public:
    CStatsBonusItem(TiXmlNode* node);
    ~CStatsBonusItem();

    virtual void loadResources();
    virtual void releaseResources();
    virtual bool isAvailable() const { return false; }
    virtual void setAvailable(bool available);
    virtual com::glu::platform::graphics::ICRenderSurface* GetShopImage() { return m_shopImage; }
    virtual XString& GetName() { return m_name; }
    virtual XString& GetDescription() { return m_description; }
    virtual int getUnlockLvl() const { return 0; }
    virtual const char* GetItemTypeName() const { return "statbonusitem"; }
    virtual int getId() const { return m_id; }
    virtual int Buy();
    virtual bool CheckBuyConditions(CZombieDialogWindow** dialog);

    void parseNode(TiXmlNode* node);
    void parseTexts(TiXmlNode* node);
    void parseStats(TiXmlNode* node);
    void parseGraphics(TiXmlNode* node);

    // Former m_data80[0xf4 - 0x80]: layout not known yet (size from CEnergyManager::AddItem).
    PlayerStats m_stats;                    // 0x80
    int m_iapPrice;                         // 0xd0
    int m_id;                               // 0xd4
    bool m_fieldD8;                         // 0xd8 set false by constructor, meaning unresolved
    const char* m_bigImageResource;         // 0xdc
    com::glu::platform::graphics::ICRenderSurface* m_bigImage;   // 0xe0
    const char* m_shopImageResource;        // 0xe4
    com::glu::platform::graphics::ICRenderSurface* m_shopImage;  // 0xe8
    XString m_description;                  // 0xec
    XString m_name;                         // 0xf0
};

#endif
