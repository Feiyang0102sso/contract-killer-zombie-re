// k: complete paging/creator owner. The original dual -1/1/0 states distinguish the previous page from the current button on overflow.
#include "CShopTabContentWindow.h"
#include "CShopItemButton.h"
#include "CShopItem.h"
#include "CAutoArrangedWindow.h"

class ManagerItemButton : public CShopTabContentWindow::ItemButtonCreator {
public:
    ManagerItemButton(CItemManager* manager, CItem* item) : m_manager(manager), m_item(item) {}
    virtual ~ManagerItemButton() {}
    virtual CShopItemButton* create()
    {
        CItemManager* manager = m_manager;
        CShopItemButton* button = manager->CreateShopItemButton(m_item->getId());
        if (button != NULL) {
            button->m_command = 0x30471317;
            button->m_commandParameter1 = (int)button;
            button->m_commandParameter2 = 0;
            button->m_commandParameter3 = 0;
        }
        return button;
    }
    virtual bool filter(int value) { return !m_item->CheckFilter(value); }
    virtual bool show() const
    {
        if (m_item->isHidden() != 1) {
            return true;
        }
        if (m_item->IsOnSale()) {
            return true;
        }
        return m_item->IsOwned();
    }
    virtual int getId() const { return m_item->getId(); }
    CItemManager* m_manager; // 4
    CItem* m_item; // 8
};

CShopTabContentWindow::CShopTabContentWindow(int width, int height)
    : CShopScrollWindow(), m_focusedItem(-1), m_itemWidth(width), m_itemHeight(height)
{
    UpdateContent(-1);
}

void CShopTabContentWindow::UpdateContent(int filter)
{
    FlushPages();
    CreatePage(NULL, filter);
    SetCurrPageIdx(GetCurrPageIdx());
    if (m_focusedItem >= 0) {
        m_focusedItem = -1;
    }
}

CShopTabContentWindow::~CShopTabContentWindow()
{
    ClearContents();
}

void CShopTabContentWindow::ClearContents()
{
    for (int index = 0; index < m_creators.size(); index++) {
        if (m_creators[index] != NULL) {
            delete m_creators[index];
        }
    }
    m_creators.removeAllElements();
}

CAutoArrangedWindow* CShopTabContentWindow::CreatePage(CAutoArrangedWindow* page, int filter)
{
    if (page == NULL) {
        page = CreateItemsContainer();
    }
    int featured = -1;
    int focused = -1;
    for (int index = 0; index < m_creators.size(); index++) {
        ItemButtonCreator* creator = m_creators[index];
        if (creator->show() != 1 || creator->filter(filter) != 0) {
            continue;
        }
        CShopItemButton* button = creator->create();
        if (button == NULL) {
            continue;
        }
        int id = creator->getId();
        bool blinking = button->m_flagEc;
        int focusId = m_focusedItem;
        bool added = page->Add(button);
        if (id == focusId) {
            focused = 1;
        }
        if (blinking) {
            featured = 1;
        }
        if (!added) {
            Add(page, featured == 0, focused == 0);
            page = CreateItemsContainer();
            page->Add(button);
            if (focused == 0) {
                focused = -1;
            }
            if (featured == 0) {
                featured = -1;
            }
        }
        if (featured == 1) {
            featured = 0;
        }
        if (focused == 1) {
            focused = 0;
        }
    }
    Add(page, featured == 0, focused == 0);
    return page;
}

void CShopTabContentWindow::AddButton(ItemButtonCreator* creator, bool append)
{
    if (append == 1) {
        m_creators.addElement(creator);
    } else {
        m_creators.insertElementAt(creator, 0);
    }
}

void CShopTabContentWindow::AddManagerContents(CItemManager* manager)
{
    if (manager->GetItemsCount() != 0) {
        CItem* item = manager->firstItem();
        while (item != NULL) {
            ItemButtonCreator* creator = new ManagerItemButton(manager, item);
            m_creators.addElement(creator);
            item = manager->nextItem();
        }
    }
}

CAutoArrangedWindow* CShopTabContentWindow::CreateItemsContainer()
{
    CAutoArrangedWindow* page = new CAutoArrangedWindow(m_itemWidth, m_itemHeight);
    page->m_maxRows = 2;
    page->SetCorner(0, 0);
    return page;
}

bool CShopTabContentWindow::IsFilterFilterContainsFocusedItem(int filter)
{
    for (int index = 0; index < m_creators.size(); index++) {
        ItemButtonCreator* creator = m_creators[index];
        if (creator->show() == 1 && creator->filter(filter) == 0
            && creator->getId() == m_focusedItem) {
            return true;
        }
    }
    return false;
}
