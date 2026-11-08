// k: complete Rect owner; the pure-virtual info window, static factory and local inventory counter class all follow the original refs.
#include "CShopRectItemButton.h"
#include "SGImageWindow.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "ServiceWindows.h"

CShopRectItemButton::CShopRectItemButton(CItem* item) : CShopItemButton(item)
{
    m_rectItem = item;
    SGImageWindow* background = new SGImageWindow();
    background->SetArchetypeCharacter(1, 0);
    background->SetAnimation(26, 1);
    AddToFront(background);
    SetSize(background->m_imageWidth, background->m_imageHeight);
    Window* priceContainer = new WindowTransparent(false);
    priceContainer->SetAlign(0x14);
    priceContainer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(4)(), 0);
    priceContainer->SetPercentWidth(20, 0, 0);
    priceContainer->SetHeightByContent(0, 0);
    int points = item->getPricePoints();
    m_priceText = new TextWindow();
    {
        int price;
        if (points > 0) {
            price = item->getPricePoints();
        } else {
            price = item->getPriceCredits();
        }
        XString value(price);
        m_priceText->SetText(value, CFontMgr::GetInstance()->GetFont(25));
        m_priceText->SetAlign(0x22);
        m_priceText->SetOutsetSpacing(SScreenAdaptingConstant(3)(), 0, 0, 0);
        priceContainer->AddToFront(m_priceText);
        SGImageWindow* currency = new SGImageWindow();
        currency->SetArchetypeCharacter(1, 0);
        int animation = 5;
        if (points > 0) {
            animation = 6;
        }
        currency->SetAnimation(animation, 1);
        currency->SetAlign(0x21);
        priceContainer->AddToFront(currency);
    }
    AddToFront(priceContainer);
    Window* nameContainer = new WindowTransparent(false);
    nameContainer->SetAlign(10);
    int first = SScreenAdaptingConstant(4)();
    int second = SScreenAdaptingConstant(5)();
    nameContainer->SetOutsetSpacing(first, second, 0, 0);
    nameContainer->SetPercentWidth(50, 0, 0);
    nameContainer->SetHeightByContent(0, 0);
    TextWindow* name = new TextWindow();
    XString& title = item->GetName();
    name->SetText(title, CFontMgr::GetInstance()->GetFont(26));
    name->SetAlign(0x24);
    nameContainer->AddToFront(name);
    AddToFront(nameContainer);
    // The original base constructor does not initialize f4; the concrete leaf classes create the image afterwards.
}

Window* CShopRectItemButton::CreateStatTextWindow(XString first, XString second)
{
    Window* container = new WindowTransparent(false);
    container->SetWidthByContent(0, 0);
    container->SetHeightByContent(0, 0);
    TextWindow* label = new TextWindow();
    label->SetText(first + L":", CFontMgr::GetInstance()->GetFont(23));
    label->SetCellPos(0, 0, 1, 1);
    TextWindow* value = new TextWindow();
    value->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(5)());
    value->SetAlign(1);
    value->SetWidthByContent(0, 0);
    value->SetHeightByContent(0, 0);
    value->SetText(second, CFontMgr::GetInstance()->GetFont(25));
    container->AddToFront(createTD(value, 3), 1, 0);
    container->AddToFront(label);
    container->SetAlign(2);
    return container;
}

Window* CShopRectItemButton::CreateInInventoryCountPanel(CItem* item)
{
    class ItemCountTextWindow : public TextWindow {
    public:
        explicit ItemCountTextWindow(CItem* item)
            : TextWindow(CFontMgr::GetInstance()->GetFont(27))
        {
            m_count = item->GetCount() + 1;
            m_item = item;
        }
        virtual ~ItemCountTextWindow() {}
        virtual void Update()
        {
            if (m_item->GetCount() != m_count) {
                m_count = m_item->GetCount();
                SetText(XString(m_count));
            }
        }
        int m_count; // d8, the original initial value is 1 more than the real inventory, so the first Update always refreshes.
        CItem* m_item; // dc
    };

    SGImageWindow* container = new SGImageWindow();
    container->SetArchetypeCharacter(1, 0);
    container->SetAnimation(85, 1);
    int second = SScreenAdaptingConstant(5)();
    int fourth = SScreenAdaptingConstant(4, 9)();
    container->SetInsetSpacing(0, second, 0, fourth);
    TextWindow* label = new TextWindow();
    label->SetText(ResString("IDS_SHOP_GRENADE_INVENTORY"), CFontMgr::GetInstance()->GetFont(24));
    label->SetAlign(0x24);
    container->AddToFront(label, 0, 0);
    ItemCountTextWindow* count = new ItemCountTextWindow(item);
    count->SetAlign(0x24);
    container->AddToFront(count, 1, 0);
    return container;
}

void CShopRectItemButton::AddInInventoryCountPanel()
{
    Window* inventory = CreateInInventoryCountPanel(m_rectItem);
    inventory->SetAlign(9);
    inventory->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(50)());
    AddToFront(inventory);
}

void CShopRectItemButton::AddStrips()
{
    if (m_rectItem->IsEquiped() == 1) {
        AddEquipStrip(48, ResString("IDS_WEAPON_EQUIPPED"), 29);
    } else if (m_rectItem->IsLocked() == 1) {
        XString format = ResString("IDS_WEAPON_LEVEL_LOCK");
        AddEquipStrip(47, XString::Format(format.m_data, m_rectItem->getUnlockLvl()), 27);
    } else if (m_rectItem->IsOwned() == 1) {
        AddEquipStrip(49, ResString("IDS_WEAPON_OWNED"), 29);
    }
}

void CShopRectItemButton::AddEquipStrip(int animation, const XString& text, int font)
{
    if (animation >= 0) {
        SGImageWindow* strip = new SGImageWindow();
        strip->SetArchetypeCharacter(1, 0);
        strip->SetAnimation(animation, 1);
        strip->SetTranslateCoords(false);
        strip->SetLayoutType(LAYOUT_SIMPLE);
        strip->SetAlign(9);
        AddToFront(strip);
    }
    if (text.ByteSize() != 0) {
        TextWindow* label = new TextWindow();
        label->SetText(text, CFontMgr::GetInstance()->GetFont(font));
        label->SetAlign(0x14);
        int third = SScreenAdaptingConstant(19)();
        int fourth = SScreenAdaptingConstant(8)();
        label->SetOutsetSpacing(0, 0, third, fourth);
        AddToFront(label);
    }
}

void CShopRectItemButton::AddMarkers()
{
    if (AddHotMarker() == NULL) {
        AddNewMarker();
    }
    Window* sale = AddSalesMarker();
    if (sale != NULL) {
        sale->SetOutsetSpacing(0, 0, 0, SScreenAdaptingConstant(-5)());
        sale->SetFlags(0x800000);
    }
}

void CShopRectItemButton::Update()
{
    if (m_pressed) {
        SetScale(0.94f);
    } else {
        SetScale(1.0f);
    }
    CShopItemButton::Update();
}

CShopRectItemButton::~CShopRectItemButton()
{
}

void CShopRectItemButton::SetPrice(int price)
{
    m_priceText->SetText(XString(price));
}
