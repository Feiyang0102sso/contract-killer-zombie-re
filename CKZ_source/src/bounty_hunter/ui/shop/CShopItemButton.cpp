// k: original 15cd28..15d6d4 complete owner; the local timer keeps its original scope and string lifetime.
#include "CShopItemButton.h"
#include "SGImageWindow.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "CStdUtil_Cocoa.h"
#include "MathLib.h"

CShopItemButton::CShopItemButton(CItem* item) : CBH_Button()
{
    m_item = item;
    m_price = 0;
    m_flagEc = false;
    if (item != NULL) {
        m_price = GetPrice();
        m_cachedPrice = m_price;
        SetPrice(m_price);
    }
    SetWidthByContent(0, 0);
}

int CShopItemButton::GetPrice()
{
    if (m_item == NULL) {
        return 0;
    }
    if (m_item->getPriceCredits() >= 1) {
        return m_item->getPriceCredits();
    }
    return m_item->getPricePoints();
}

int CShopItemButton::Buy()
{
    if (m_item != NULL) {
        return m_item->Buy();
    }
    return 0;
}

bool CShopItemButton::CheckBuyConditions(CZombieDialogWindow** dialog)
{
    if (m_item != NULL) {
        return m_item->CheckBuyConditions(dialog);
    }
    return false;
}

void CShopItemButton::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    Window::Paint(graphics);
}

void CShopItemButton::Update()
{
    if (m_pressed) {
        SetOpacity(0.7f);
    } else {
        SetOpacity(1.0f);
    }
    if (m_flagEc) {
        unsigned int time = (unsigned int)CStdUtil_Cocoa::GetUpTimeMS();
        float phase = (int)(time % 600) - 300;
        SetOpacity(MathLib::Fabs(phase) / 360.0f + 0.2f);
    }
    if (GetPrice() != m_cachedPrice) {
        m_cachedPrice = GetPrice();
        SetPrice(m_cachedPrice);
    }
}

Window* CShopItemButton::AddHotMarker()
{
    if (m_item == NULL || m_item->IsHot() != 1) {
        return NULL;
    }
    SGImageWindow* marker = new SGImageWindow();
    marker->SetArchetypeCharacter(1, 0);
    marker->SetAnimation(58, 1);
    TextWindow* text = new TextWindow();
    text->SetText(ResString("IDS_SHOP_HOT"), CFontMgr::GetInstance()->GetFont(29));
    text->SetAlign(0x24);
    int first = SScreenAdaptingConstant(2)();
    int last = SScreenAdaptingConstant(2)();
    text->SetOutsetSpacing(first, 0, 0, last);
    marker->AddToFront(text);
    marker->SetLayoutType(LAYOUT_SIMPLE);
    marker->SetAlign(9);
    first = SScreenAdaptingConstant(3)();
    last = SScreenAdaptingConstant(-10)();
    marker->SetOutsetSpacing(first, 0, 0, last);
    marker->SetFlags(0x800000);
    AddToFront(marker);
    return marker;
}

Window* CShopItemButton::AddNewMarker()
{
    if (m_item == NULL || m_item->IsNew() != 1) {
        return NULL;
    }
    SGImageWindow* marker = new SGImageWindow();
    marker->SetArchetypeCharacter(1, 0);
    marker->SetAnimation(86, 1);
    TextWindow* text = new TextWindow();
    text->SetText(ResString("IDS_SHOP_NEW"), CFontMgr::GetInstance()->GetFont(29));
    text->SetAlign(0x24);
    int first = SScreenAdaptingConstant(2)();
    int last = SScreenAdaptingConstant(2)();
    text->SetOutsetSpacing(first, 0, 0, last);
    marker->AddToFront(text);
    marker->SetLayoutType(LAYOUT_SIMPLE);
    marker->SetAlign(9);
    first = SScreenAdaptingConstant(3)();
    last = SScreenAdaptingConstant(-10)();
    marker->SetOutsetSpacing(first, 0, 0, last);
    marker->SetFlags(0x800000);
    AddToFront(marker);
    return marker;
}

Window* CShopItemButton::AddSalesMarker()
{
    class SmartTimerText : public SGImageWindow {
    public:
        explicit SmartTimerText(CItem* item) : m_item(item) {}
        virtual ~SmartTimerText() {}
        virtual void Update()
        {
            bool sale = m_item->IsOnSale();
            Window* parent = m_parent;
            if (sale != 1) {
                parent->ClearFlags(1);
                return;
            }
            parent->SetFlags(1);
            if (m_item->IsSaleWithTimer() != 1) {
                ClearFlags(1);
                return;
            }
            int remaining = m_item->GetSaleTimeLeft();
            if (remaining < 1) {
                ClearFlags(1);
                return;
            }
            SetFlags(1);
            int days = remaining / 86400;
            XString value;
            unsigned int time = (unsigned int)CStdUtil_Cocoa::GetUpTimeMS();
            int separator = ' ';
            if (time % 1000 >= 500) {
                separator = ':';
            }
            if (remaining >= 172800) {
                value = XString(days) + XString(L" ") + ResString("IDS_SHOP_SALE_DAYS");
            } else if ((unsigned int)(remaining - 86400) < 86400) {
                value = XString(days) + XString(L" ") + ResString("IDS_SHOP_SALE_DAY");
            } else {
                int seconds = remaining - days * 86400;
                int hours = seconds / 3600;
                int minutes = (seconds - hours * 3600) / 60;
                if (seconds >= 3600) {
                    value = XString::Format(L"%02d%c%02d", hours, separator, minutes);
                } else {
                    value = XString::Format(L"%02d%c%02d", minutes, separator,
                                            seconds - hours * 3600 - minutes * 60);
                }
            }
            bool same;
            {
                XString current = m_text->m_text;
                same = current == value;
            }
            if (!same) {
                m_text->SetText(value);
            }
        }
        CItem* m_item; // 10c
        TextWindow* m_text; // 110
    };

    if (m_item == NULL || m_item->IsSale() != 1) {
        return NULL;
    }
    Window* container = new WindowTransparent(false);
    container->SetDesiredWidth(SScreenAdaptingConstant(86)());
    container->SetHeightByContent(0, 0);
    SGImageWindow* label = new SGImageWindow();
    label->SetArchetypeCharacter(1, 0);
    label->SetAnimation(56, 1);
    label->SetAlign(0x21);
    TextWindow* text = new TextWindow();
    text->SetText(ResString("IDS_SHOP_SALE"), CFontMgr::GetInstance()->GetFont(29));
    text->SetAlign(0x24);
    text->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(2)(), 0);
    label->AddToFront(text);
    container->AddToFront(label);
    SmartTimerText* timer = new SmartTimerText(m_item);
    timer->SetArchetypeCharacter(1, 0);
    timer->SetAnimation(57, 1);
    timer->m_text = new TextWindow(CFontMgr::GetInstance()->GetFont(24));
    timer->m_text->SetAlign(0x24);
    timer->AddToFront(timer->m_text);
    timer->SetInsetSpacing(0, SScreenAdaptingConstant(1)());
    timer->SetOutsetSpacing(SScreenAdaptingConstant(2)(), 0, 0, 0);
    timer->SetAlign(10);
    container->AddToFront(timer);
    container->SetLayoutType(LAYOUT_SIMPLE);
    container->SetAlign(0x11);
    AddToFront(container);
    return container;
}

CShopItemButton::~CShopItemButton()
{
}
