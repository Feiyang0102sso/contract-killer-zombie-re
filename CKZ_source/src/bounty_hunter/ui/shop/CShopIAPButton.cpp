// k: original 15cbc8..15cd28 complete IAP button; the store price is filled in by Currency.
#include "CShopIAPButton.h"
#include "CIAPCurrency.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"

CShopIAPButton::CShopIAPButton(CIAPCurrency* currency) : CShopCommonItemButton(currency)
{
    m_priceContainer->ClearFlags(1);
    if (currency->m_badgeText.ByteSize() != 0) {
        TextWindow* badge = new TextWindow();
        badge->SetText(currency->m_badgeText, CFontMgr::GetInstance()->GetFont(24));
        badge->SetAlign(0x14);
        badge->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(5)(), 0);
        AddToFront(badge);
    }
    TextWindow* price = new TextWindow();
    currency->FillPriceText(price);
    price->SetOutsetSpacing(SScreenAdaptingConstant(22)(), 0, 0, 0);
    price->SetAlign(0xc);
    AddToFront(price);
}

CShopIAPButton::~CShopIAPButton()
{
}
