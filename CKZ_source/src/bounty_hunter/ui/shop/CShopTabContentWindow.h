// k: original 15e5ec complete owner, Scroll base 130, real creator virtual interface and Vector entities.
#ifndef CSHOPTABCONTENTWINDOW_H
#define CSHOPTABCONTENTWINDOW_H
#include "CShopScrollWindow.h"
class CAutoArrangedWindow;
class CShopItemButton;
class CItemManager;
class CShopTabContentWindow : public CShopScrollWindow {
public:
    class ItemButtonCreator {
    public:
        virtual ~ItemButtonCreator() {}
        virtual CShopItemButton* create() = 0;
        virtual bool filter(int value) = 0;
        virtual bool show() const = 0;
        virtual int getId() const = 0;
    };
    CShopTabContentWindow(int width, int height);
    virtual ~CShopTabContentWindow();
    void UpdateContent(int filter);
    void ClearContents();
    CAutoArrangedWindow* CreatePage(CAutoArrangedWindow* page, int filter);
    void AddButton(ItemButtonCreator* creator, bool append);
    void AddManagerContents(CItemManager* manager);
    CAutoArrangedWindow* CreateItemsContainer();
    bool IsFilterFilterContainsFocusedItem(int filter);
    int m_focusedItem; // 130
    int m_itemWidth; // 134
    int m_itemHeight; // 138
    Vector<ItemButtonCreator*> m_creators; // 13c, total 14c; owns the creator.
};
#endif
