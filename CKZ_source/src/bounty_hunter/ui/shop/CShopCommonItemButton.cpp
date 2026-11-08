// k: original 15be60..15c270 complete Common button; child is owned by the Window tree.
#include "CShopCommonItemButton.h"
#include "SGImageWindow.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "CZombieDialogWindow.h"

CShopCommonItemButton::CShopCommonItemButton(CItem* item) : CShopItemButton(item)
{
    SGImageWindow* background = new SGImageWindow();
    background->SetArchetypeCharacter(1, 0);
    background->SetAnimation(27, 1);
    AddToFront(background);
    SetSize(background->m_imageWidth, background->m_imageHeight);
    m_priceContainer = new WindowTransparent(false);
    m_priceContainer->SetAlign(0x14);
    m_priceContainer->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(4)(), 0);
    m_priceContainer->SetPercentWidth(70, 0, 0);
    m_priceContainer->SetHeightByContent(0, 0);
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
        m_priceContainer->AddToFront(m_priceText);
        SGImageWindow* currency = new SGImageWindow();
        currency->SetArchetypeCharacter(1, 0);
        int animation = 5;
        if (points > 0) {
            animation = 6;
        }
        currency->SetAnimation(animation, 1);
        currency->SetAlign(0x21);
        m_priceContainer->AddToFront(currency);
    }
    AddToFront(m_priceContainer);
    ImageWindow* image = new ImageWindow();
    image->SetImage(item->GetShopImage());
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(0x14);
    image->SetOutsetSpacing(0, 0, SScreenAdaptingConstant(10)(), 0);
    AddToFront(image);
    Window* nameContainer = new WindowTransparent(false);
    nameContainer->SetAlign(0xc);
    nameContainer->SetOutsetSpacing(SScreenAdaptingConstant(4)(), 0, 0, 0);
    nameContainer->SetPercentWidth(90, 0, 0);
    nameContainer->SetHeightByContent(0, 0);
    TextWindow* name = new TextWindow();
    XString& title = item->GetName();
    name->SetText(title, CFontMgr::GetInstance()->GetFont(26));
    name->SetAlign(0x24);
    name->SetPercentWidth(100, 0, 0);
    name->SetHeightByContent(0, 0);
    nameContainer->AddToFront(name);
    AddToFront(nameContainer);
}

Window* CShopCommonItemButton::CreateInfoDialog()
{
    CZombieDialogWindow* dialog = new CZombieDialogWindow(3);
    dialog->SetTitle(m_item->GetName());
    dialog->SetText(m_item->GetDescription(), false, NULL);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)5, 0x3b7082f1,
                      (int)this, 0, true, 20);
    dialog->AddButton((CZombieDialogWindow::EDialogButton)4, -1, 0, 0, true, 20);
    return dialog;
}

void CShopCommonItemButton::SetPrice(int price)
{
    m_priceText->SetText(XString(price));
}

void CShopCommonItemButton::Paint(com::glu::platform::graphics::ICGraphics2d& graphics)
{
    CShopItemButton::Paint(graphics);
}

void CShopCommonItemButton::Update()
{
    CShopItemButton::Update();
}

CShopCommonItemButton::~CShopCommonItemButton()
{
}
