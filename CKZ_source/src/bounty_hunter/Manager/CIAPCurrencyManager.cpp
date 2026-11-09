// CIAPCurrencyManager.cpp - store currency packs and product-data requests.
#include "CIAPCurrencyManager.h"
#include "CIAPCurrency.h"
#include "CShopIAPButton.h"
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;

CIAPCurrencyManager::CIAPCurrencyManager()
{
    m_requestState = 0;
    LoadFromConfig("res/common/configs/iapcurrency.xml", "currencies", "currency");
}

void CIAPCurrencyManager::AddItem(TiXmlNode* node)
{
    CIAPCurrency* item = new CIAPCurrency(node);
    m_items.addElement(item);
}

CIAPCurrencyManager::~CIAPCurrencyManager()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (m_items[index] != NULL) {
            delete m_items[index];
            m_items[index] = NULL;
        }
    }
}

CIAPCurrency* CIAPCurrencyManager::getIAPCurrencyByAppleId(XString id) const
{
    for (int index = 0; index < m_items.size(); index++) {
        if (XString(m_items[index]->m_appleId) == id) {
            return m_items[index];
        }
    }
    return NULL;
}

int CIAPCurrencyManager::GetItemsCount()
{
    return m_items.size();
}

int CIAPCurrencyManager::GetAvailableItemsCount()
{
    int count = 0;
    for (int index = 0; index < m_items.size(); index++) {
        if (!(XString(m_items[index]->m_type) == L"tapjoy")) {
            count += m_items[index]->isAvailable();
        }
    }
    return count;
}

void CIAPCurrencyManager::GetAvailableCurrencies(Vector<CIAPCurrency*>& items)
{
    items.removeAllElements();
    for (int index = 0; index < m_items.size(); index++) {
        // Original @0x12ac3a releases the copied string before the virtual availability query.
        bool isTapjoy = XString(m_items[index]->m_type) == L"tapjoy";
        if (!isTapjoy && m_items[index]->isAvailable() == true) {
            items.addElement(m_items[index]);
        }
    }
}

void CIAPCurrencyManager::SetAllUnavailable()
{
    for (int index = 0; index < m_items.size(); index++) {
        if (!(XString(m_items[index]->m_type) == L"tapjoy")) {
            m_items[index]->setAvailable(false);
        }
    }
}

void CIAPCurrencyManager::loadResources()
{
    for (int index = 0; index < m_items.size(); index++) {
        m_items[index]->loadResources();
    }
}

void CIAPCurrencyManager::UpdateIAPInfo(bool force)
{
    // The shipped implementation ignores force and always uses a 60000 ms request timeout.
    com::glu::platform::framework::CApp* app = CApplet::m_pApp;
    void* found = NULL;
    unsigned int classId = ClassId_ICInAppPurchase;
    app->m_singletons->Find(classId, &found);
    ICInAppPurchase* store = static_cast<ICInAppPurchase*>(found);
    if (store == NULL) {
        store = ICInAppPurchase::CreateInstance();
    }
    CVector<CStrWChar> products;
    for (int index = 0; index < m_items.size(); index++) {
        XString appleId = m_items[index]->m_appleId;
        const wchar_t* productId = appleId.c_str();
        if (productId != NULL) {
            products.Add(CStrWChar(productId));
        }
    }
    store->RequestProductData(products, 60000);
}

void CIAPCurrencyManager::releaseResources()
{
    for (int index = 0; index < m_items.size(); index++) {
        m_items[index]->releaseResources();
    }
}

CItem* CIAPCurrencyManager::getItemByInternalId(int index)
{
    CItem* item = NULL;
    if (index < m_items.size()) {
        item = m_items[index];
    }
    return item;
}

CShopItemButton* CIAPCurrencyManager::CreateShopItemButton(int index)
{
    return new CShopIAPButton(static_cast<CIAPCurrency*>(getItemById(index)));
}
