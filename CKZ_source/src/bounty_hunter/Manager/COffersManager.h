// PROVISIONAL (created by b057x): owner src/bounty_hunter/Manager/COffersManager.cpp not written yet
// b078c: implementation recovered; the preceding PROVISIONAL comment is historical.
// COffersManager.h - special offers and daily deals; only what src/general/App.cpp uses is declared.
// b078c: record is 0x1c (not the provisional 0x10), manager is 0x6c.
#ifndef COFFERSMANAGER_H
#define COFFERSMANAGER_H

#include <stddef.h>
#include "Vector_ss.h"
#include "xString.h"
class TiXmlNode;
class TiXmlElement;
class COneTimeOfferDialogWindow;
class CDailyDealDialogWindow;

/** A daily deal (defined in COffersManager.cpp). */
struct SDailyDealOffer {
    SDailyDealOffer()
    {
        m_itemUid = m_discount = XString("");
        m_newPrice = 0;
        m_endTime = 0;
        m_startTime = 0;
        m_id = 0;
        m_wasPurchased = true;
    }
    SDailyDealOffer(int id, XString itemUid, int start, int end, int newPrice, XString discount)
    {
        m_id = id;
        m_itemUid = itemUid;
        m_newPrice = newPrice;
        m_startTime = start;
        m_endTime = end;
        m_wasPurchased = false;
        m_discount = discount;
    }
    ~SDailyDealOffer() {} // Member strings perform the original destruction.
    XString GetID()
    {
        return m_itemUid + XString("_") + XString((int)m_startTime);
    }
    bool IsValid()
    {
        return m_itemUid.ByteSize() != 0 && m_startTime != 0 && m_endTime > m_startTime &&
               m_discount.ByteSize() != 0 && m_id > 0;
    }

    // Former m_data[4]: provisional contents and size were not known.
    int m_id;                 // 0x00
    XString m_itemUid;         // 0x04
    unsigned int m_startTime;  // 0x08
    unsigned int m_endTime;    // 0x0c
    XString m_discount;        // 0x10
    int m_newPrice;            // 0x14
    bool m_wasPurchased;       // 0x18
};

class COffersManager {
public:
    COffersManager();
    void ReloadConfig();
    void ParseOTO(TiXmlNode* node);
    void ParseDD(TiXmlNode* node);
    void InitiateOTOProcess();
    void ClearOffers();
    void Load(TiXmlElement* root);
    void Save(TiXmlElement* root);
    bool ShouldBeShown();
    COneTimeOfferDialogWindow* ShowOTODialog();
    SDailyDealOffer* GetActiveDDOffer();
    void UpdateOffers();
    void CheckAndShowDD();
    void Tick(unsigned int now);
    void DialogClosed();
    void OnDDPurchased(SDailyDealOffer* offer);
    /** Extra currency of the running one-time offer (read by CIAPCurrency::OnIAPBought at 0x14). */  // added by b077
    int GetOneTimeOfferBonus() const  // added by b077
    {  // added by b077
        return m_bonus;  // added by b077
    }  // added by b077

    // Former m_data00[0x24]: layout not known yet.
    int m_maxShows;                    // 0x00
    unsigned int m_showDelay;          // 0x04
    int m_startingLevel;               // 0x08
    float m_showProbability;           // 0x0c
    int m_goldPackId;                  // 0x10
    int m_bonus;                       // 0x14
    unsigned int m_lastOfferTime;      // 0x18
    int m_showCount;                   // 0x1c
    union {
        float m_probabilityRandom;    // 0x20
        int m_probabilityRandomBits;  // persistence stores the bits as an integer
    };
    XString m_text;                     // 0x24
    // Former m_data28[0x38 - 0x28].
    int m_configVersion;                // 0x28
    unsigned int m_dealDuration;        // 0x2c
    unsigned int m_dialogShowDelay;     // 0x30
    int m_dealStartingLevel;            // 0x34
    Vector<SDailyDealOffer> m_dailyDeals;   // 0x38
    Vector<int> m_values;               // 0x48 (element type not known in b057x; now verified offer indices)
    // Former m_data58[0x6c - 0x58].
    int m_activeDealIndex;               // 0x58 index into m_values
    bool m_offersChanged;                // 0x5c
    unsigned int m_nextPurchaseTime;     // 0x60
    unsigned int m_lastDialogTime;       // 0x64
    CDailyDealDialogWindow* m_activeDialog; // 0x68
};

#endif
