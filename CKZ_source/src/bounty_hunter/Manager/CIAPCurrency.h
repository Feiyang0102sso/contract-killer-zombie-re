// CIAPCurrency.h - a currency pack bought with an in-app purchase (an item of the shop).
#ifndef CIAPCURRENCY_H
#define CIAPCURRENCY_H

#include "CShopItem.h"

class TiXmlNode;
class TiXmlElement;
class TextWindow;

class CIAPCurrency : public CItem {
public:
    CIAPCurrency(TiXmlNode* node);
    ~CIAPCurrency();

    virtual void loadResources();                                           // 0x00
    virtual void releaseResources();                                        // 0x04
    virtual bool isHidden() const                                           // 0x08
    {
        if (m_hidden) {
            return true;
        }
        return !isAvailable();
    }
    virtual bool isAvailable() const                                        // 0x0c
    {
        return m_available;
    }
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
        return "iapcurrency";
    }
    virtual int getId() const                                               // 0x50
    {
        return m_id;
    }
    /** Start the store purchase; 0 when the product is not available (the info is requested again). */
    virtual int Buy();                                                      // 0x58

    void parseNode(TiXmlNode* node);
    void parseTexts(TiXmlElement* element);
    void parseGraphics(TiXmlElement* element);
    void parseStats(TiXmlElement* element);
    /** Buy with the bonus of the one-time offer. */
    void BuyFromOneTimeOffer();
    /** The store reported the purchase: give the currency. */
    void OnIAPBought();
    /** "<currency> <price>" (no decimals for whole prices). */
    void FillPriceText(TextWindow* text);

    const char* m_shopButton;           // 0x80 resource id
    void* m_shopImage;                  // 0x84 resource data, NULL when not loaded
    int m_field88;                      // 0x88
    int m_field8c;                      // 0x8c
    XString m_currencySymbol;           // 0x90 (store price info)
    bool m_fromOneTimeOffer;            // 0x94
    bool m_available;                   // 0x95 the store knows the product
    bool m_hiddenInShop;                // 0x96 "hidden" of <stats>
    float m_price;                      // 0x98 (store price info)
    int m_creditsGain;                  // 0x9c
    int m_cashGain;                     // 0xa0
    XString m_name;                     // 0xa4
    XString m_description;              // 0xa8
    XString m_appleId;                  // 0xac store product id
    XString m_type;                     // 0xb0
    XString m_badgeText;                // 0xb4
    int m_id;                           // 0xb8
};

#endif
