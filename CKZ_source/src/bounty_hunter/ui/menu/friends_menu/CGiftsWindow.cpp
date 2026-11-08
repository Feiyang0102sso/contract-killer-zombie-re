// j: original Gifts complete paging and the real two-Add retry boundary.
#include "CGiftsWindow.h"
#include "CAutoArrangedWindow.h"
#include "CSocialGiftsManager.h"
#include "CShopItem.h"
#include "CScrollWindow.h"
#include "CImageButton.h"
#include "BaseDialog.h"
#include "CFontMgr.h"
#include "SScreenAdaptingConstant.h"
#include "App.h"

static SScreenAdaptingConstant ARRANGED_WINDOW_HEIGHT(220, 440, 550);

CGiftsWindow::CGiftsWindow() : Window(false)
{
    Vector<const SGift*> gifts;
    GetGifts(gifts);
    CreateUserInterface(gifts);
}

void CGiftsWindow::GetGifts(Vector<const SGift*>& gifts)
{
    CSocialGiftManager* manager = ((App*)WindowApp::m_instance)->m_socialGiftManager;
    for (int index = 0; index < manager->GetGiftsCount(); ++index) {
        const SGift* gift = manager->GetGift(index);
        if (gift != NULL) {
            gifts.addElement(gift);
        }
    }
}

void CGiftsWindow::CreateUserInterface(const Vector<const SGift*>& gifts)
{
    CAutoArrangedWindow* page = new CAutoArrangedWindow(WindowApp::m_instance->m_screenWidth, ARRANGED_WINDOW_HEIGHT());
    CScrollWindow* scroll = new CScrollWindow(true);
    scroll->Add(page, false, false);
    for (int index = 0; index < gifts.m_size; ++index) {
        // The original really only used the filtered count and re-queries by consecutive index; not changed to gifts[index].
        Window* button = CreateGiftButton(index);
        if (!page->Add(button)) {
            page = new CAutoArrangedWindow(WindowApp::m_instance->m_screenWidth, ARRANGED_WINDOW_HEIGHT());
            scroll->Add(page, false, false);
            page->Add(button);
        }
    }
    AddToFront(scroll);
}

Window* CGiftsWindow::CreateGiftButton(int index)
{
    SGift* gift = ((App*)WindowApp::m_instance)->m_socialGiftManager->GetGift(index);
    CItem* item = gift->m_itemManager->getItemById(gift->m_itemId);
    Window* content = new Window(false);
    content->SetHeightByContent(0, 0);
    content->SetWidthByContent(0, 0);
    CImageButton* image = new CImageButton;
    image->AddImage(item->GetShopImage(), 0, 0);
    image->m_command = (int)0xa4377002u;
    image->m_commandParameter1 = index;
    image->m_commandParameter2 = 0;
    image->m_commandParameter3 = 0;
    content->SetSize(image->m_width, image->m_height);
    content->AddToFront(image, 0, 0);
    TextWindow* name = new TextWindow(item->GetName(), CFontMgr::GetInstance()->GetFont(15));
    name->SetDesiredWidth(image->m_width);
    name->SetAlign(0x24);
    content->AddToFront(name, 0, 1);
    content->HandleUpdate(false);
    return content;
}
