// SimpleDialog.cpp - list dialog: a title, a text, a list of selectable items (with optional
// sub items chosen by left / right arrows) and a frame drawn from border images.
#include "SimpleDialog.h"
#include "App.h"
#include "WindowApp.h"
#include "CFontMgr.h"
#include "CUtility.h"
#include "CFont.h"
#include "ICGraphics2d.h"
#include "ICRenderSurface.h"

using namespace com::glu::platform::graphics;

/** Commands of the arrow keys / soft keys. */
static const int COMMAND_UP = 0x3f972120;           // previous item
static const int COMMAND_DOWN = (int)0x972028c1;    // next item
static const int COMMAND_LEFT = (int)0x9720a3cb;    // previous sub item
static const int COMMAND_RIGHT = 0x7214ff0d;        // next sub item
static const int COMMAND_SELECT = 0x2074d009;
static const int COMMAND_BACK = (int)0x97204784;
/** Easing command that closes the dialog. */
static const int COMMAND_CLOSE = 0x200;
/** Fonts (CFontMgr table indices). */
static const int FONT_TITLE = 0;
static const int FONT_TEXT = 1;
static const int FONT_ITEM_PRESSED = 5;
static const int FONT_ITEM = 6;
/** Extra height of an item and room left between the sub item arrows. */
static const int ITEM_MARGIN = 10;
static const int ARROWS_MARGIN = 10;
/** Room kept next to the title. */
static const int TITLE_EXTRA_WIDTH = 0x34;
/** Title: text position, frame offset below it. */
static const int TITLE_X = 0x1a;
static const int TITLE_Y = 0x14;
static const int TITLE_FRAME_TOP = 10;
/** Window flags without a name in WindowBase.h: the scroll bar is drawn / the back key cancels. */
static const int FLAG_SCROLLBAR = 0x8000;
static const int FLAG_CANCELABLE = 0x10000;
/** AdjustByRect: layout passes while the width still shrinks. */
static const int MAX_ADJUST_PASSES = 5;

/** Integer to 16.16 fixed point; the result is passed on as a float (as in the original). */
static inline int IntToFixed(int value)
{
    return value << 16;
}

SimpleDialog::ItemsWindow::ItemsWindow()
    : Window(false)
{
    FreeLayout();
    SetAlign(ALIGN_BOTTOM);
    m_dragging = false;
    m_scrollY = 0;
    m_selectedIndex = 0;
    m_topIndex = 0;
    m_arrowLeft.Create(NULL);
    m_arrowRight.Create(NULL);
    m_lineLeft.Create(NULL);
    m_lineMiddle.Create(NULL);
    m_lineRight.Create(NULL);
}

void SimpleDialog::ItemsWindow::SetSelectedIndex(int index)
{
    int oldIndex = m_selectedIndex;
    m_selectedIndex = index;
    int count = m_items.size();
    int step = -1;
    if (oldIndex < index) {
        step = 1;
    }
    // walk in the direction of the move until an enabled item is found (wrapping around)
    for (int tries = 0; tries < count; tries++) {
        if (index < 0) {
            m_selectedIndex = count - 1;
            index = count - 1;
        } else if (index >= count) {
            index = 0;
            m_selectedIndex = 0;
        }
        if (m_items[index].m_enabled) {
            break;
        }
        index += step;
        m_selectedIndex = index;
    }
    if (oldIndex != index) {
        UpdateTopIndex();
        SimpleDialog* dialog = Dialog();
        if (dialog == NULL) {
            return;
        }
        dialog->SelectedItemChanged(m_items[m_selectedIndex].m_id);
    }
}

void SimpleDialog::ItemsWindow::UpdateTopIndex()
{
    int count = m_items.size();
    if (count < 1) {
        return;
    }
    int itemHeight = ItemFontByIndex(0)->GetFontHeight();
    int selected = m_selectedIndex;
    if (selected < count - 1) {
        int gap = 0;
        if (m_parent != NULL) {
            gap = Dialog()->m_margins[MARGIN_ITEM_GAP];
        }
        itemHeight += gap;
    }
    int visibleCount = m_height / itemHeight;
    int top = selected;
    if (selected >= m_topIndex) {
        if (selected < m_topIndex + visibleCount - 1) {
            return;
        }
        top += 1 - visibleCount;
    }
    if (count - top < visibleCount) {
        top = count - visibleCount;
    }
    if (top < 0) {
        top = 0;
    }
    m_topIndex = top;
}

void SimpleDialog::ItemsWindow::SetSubitemSelectedIndex(int index, int subIndex)
{
    if ((unsigned int)index >= (unsigned int)m_items.size()) {
        return;
    }
    Item& item = m_items[index];
    int subCount = item.m_subItems.size();
    if (subCount < 1) {
        return;
    }
    // wrap around in both directions
    int wrapped = (subIndex % subCount + subCount) % subCount;
    if (item.m_subIndex == wrapped) {
        return;
    }
    item.m_subIndex = wrapped;
    SimpleDialog* dialog = Dialog();
    if (dialog == NULL) {
        return;
    }
    dialog->SelectedSubitemChanged(item.m_id, item.m_subItems[wrapped].m_id);
}

int SimpleDialog::ItemsWindow::MaxIconWidth() const
{
    int maxWidth = 0;
    for (int index = 0; index < m_items.size(); index++) {
        const Item& item = m_items[index];
        unsigned int width;
        unsigned int height;
        if (item.m_pressedIcon.m_resource != NULL) {
            item.m_pressedIcon.GetSurface()->GetWidthAndHeight(width, height);
            if (maxWidth < (int)width) {
                maxWidth = width;
            }
        }
        if (item.m_icon.m_resource != NULL) {
            item.m_icon.GetSurface()->GetWidthAndHeight(width, height);
            if (maxWidth < (int)width) {
                maxWidth = width;
            }
        }
    }
    return maxWidth;
}

void SimpleDialog::ItemsWindow::AddItem(int id, const XString& text, const char* iconName, int type)
{
    m_items.addElement(Item(id, text, type, iconName, type != ITEM_DISABLED && type != ITEM_HIDDEN));
}

CFont* SimpleDialog::ItemsWindow::ItemFontByIndex(int index) const
{
    ItemFonts fonts = Dialog()->ItemFontsByIndex(index);
    if (WindowApp::PointerCapture(0) == this) {
        int x;
        int y;
        WindowApp::GetPointerPos(0, x, y);
        m_parent->GlobalToLocal(x, y);
        if (y >= m_y && y < m_y + m_height && x >= 0 && x < m_parent->m_width && m_selectedIndex == index) {
            return fonts.m_pressedFont;
        }
    }
    return fonts.m_font;
}

int SimpleDialog::ItemsWindow::VirtualWidth() const
{
    if (m_items.size() < 1) {
        return 0;
    }
    int arrowsWidth = ARROWS_MARGIN;
    if (m_arrowLeft.m_resource != NULL) {
        unsigned int width;
        unsigned int height;
        m_arrowLeft.GetSurface()->GetWidthAndHeight(width, height);
        int leftWidth = width;
        m_arrowRight.GetSurface()->GetWidthAndHeight(width, height);
        arrowsWidth = width + leftWidth + ARROWS_MARGIN;
    }
    int maxWidth = 0;
    for (int index = m_items.size() - 1; index >= 0; index--) {
        const Item& item = m_items[index];
        CFont* font = CFontMgr::GetInstance()->GetFont(FONT_ITEM);
        int spaceWidth = font->MeasureTextWidth(L" ", -1, -1, NULL);
        int subWidth = 0;
        for (int subIndex = 0; subIndex < item.m_subItems.size(); subIndex++) {
            int width = font->MeasureTextWidth(item.m_subItems[subIndex].m_text.c_str(), -1, -1, NULL);
            if (subWidth < width) {
                subWidth = width;
            }
        }
        if (subWidth > 0) {
            subWidth += arrowsWidth + spaceWidth * 2;
        }
        int width = font->MeasureTextWidth(item.m_text.c_str(), -1, -1, NULL) + subWidth;
        if (maxWidth < width) {
            maxWidth = width;
        }
    }
    int iconWidth = MaxIconWidth();
    if (iconWidth > 0) {
        iconWidth++;
    }
    return maxWidth + iconWidth;
}

int SimpleDialog::ItemsWindow::VirtualHeight() const
{
    int gap = 0;
    if (m_parent != NULL) {
        gap = Dialog()->m_margins[MARGIN_ITEM_GAP];
    }
    int count = m_items.size();
    int height = 0;
    for (int index = 0; index < count; index++) {
        height += getItemHeight();
        if (index < count - 1) {
            height += gap;
        }
    }
    return height;
}

int SimpleDialog::getItemHeight()
{
    return CFontMgr::GetInstance()->GetFont(FONT_ITEM)->GetFontHeight() + ITEM_MARGIN;
}

void SimpleDialog::ItemsWindow::UpdateScrollYByTopIndex()
{
    int scrollY = 0;
    if (m_topIndex > 0) {
        SimpleDialog* dialog = Dialog();
        for (int index = 0; index < m_topIndex; index++) {
            ItemFontByIndex(index);
            scrollY += getItemHeight() + dialog->m_margins[MARGIN_ITEM_GAP];
        }
    }
    int virtualHeight = VirtualHeight();
    if (virtualHeight - scrollY < m_height) {
        scrollY = virtualHeight - m_height;
    }
    if (scrollY < 0) {
        scrollY = 0;
    }
    m_scrollY = scrollY;
}

void SimpleDialog::ItemsWindow::SetRect(int x, int y, int width, int height, bool force)
{
    Window::SetRect(x, y, width, height, force);
    m_topIndex = 0;
    UpdateTopIndex();
}

float SimpleDialog::ItemsWindow::ScrollPos() const
{
    if (m_height < VirtualHeight()) {
        return (float)m_scrollY / (float)(VirtualHeight() - m_height);
    }
    return 1.0f;
}

void SimpleDialog::PaintItemIcon(ICGraphics2d& graphics, int index, bool selected, Item* item, int x, int y)
{
    bool pressed = false;
    if (selected == true && WindowApp::PointerCapture(0) == m_itemsWindow) {
        int pointerX;
        int pointerY;
        WindowApp::GetPointerPos(0, pointerX, pointerY);
        GlobalToLocal(pointerX, pointerY);
        if (pointerY >= m_itemsWindow->m_y && pointerY < m_itemsWindow->m_y + m_itemsWindow->m_height &&
            pointerX >= 0 && pointerX < m_width) {
            pressed = selected;
        }
    }
    ICRenderSurface* surface;
    if (pressed) {
        surface = item->m_pressedIcon.GetSurface();
    } else {
        surface = item->m_icon.GetSurface();
    }
    if (surface == NULL) {
        return;
    }
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    graphics.PushTransform();
    graphics.Translate(IntToFixed((x - (int)width) / 2), IntToFixed(y - height / 2));
    graphics.Draw(*surface, NULL, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
    graphics.PopTransform();
}

void SimpleDialog::ItemsWindow::PaintItem(ICGraphics2d& graphics, int index, int x, int y, int height, bool centered)
{
    CFont* font = ItemFontByIndex(index);
    const Item& item = m_items[index];
    if (index < m_items.size() - 1) {
        // separator line below the item
        if (centered == true) {
            DrawLineImageHoriz(graphics, m_lineLeft.GetSurface(), m_lineMiddle.GetSurface(), m_lineRight.GetSurface(), 0,
                               y + height, m_width);
        } else {
            DrawLineImageHoriz(graphics, m_lineMiddle.GetSurface(), m_lineMiddle.GetSurface(), m_lineRight.GetSurface(),
                               0, y + height, m_width / 2);
        }
    }
    int align = ALIGN_LEFT | ALIGN_VCENTER;
    if (centered) {
        align = ALIGN_HCENTER | ALIGN_VCENTER;
    }
    DrawFontText(graphics, font, item.m_text.c_str(), x, y + height / 2, align);
}

void SimpleDialog::ItemsWindow::PaintSubItem(ICGraphics2d& graphics, int index, int x, int y)
{
    CFont* font = ItemFontByIndex(index);
    Item& item = m_items[index];
    int fontHeight = font->GetFontHeight();
    int selected = m_selectedIndex;
    int leftWidth = 0;
    int leftHalfHeight = 0;
    int rightWidth = 0;
    int rightHalfHeight = 0;
    if (m_arrowLeft.m_resource != NULL) {
        unsigned int width;
        unsigned int height;
        m_arrowLeft.GetSurface()->GetWidthAndHeight(width, height);
        leftWidth = width;
        leftHalfHeight = (int)height;
        m_arrowRight.GetSurface()->GetWidthAndHeight(width, height);
        leftHalfHeight = leftHalfHeight / 2;
        rightHalfHeight = (int)height / 2;
        rightWidth = width;
    }
    if (item.m_subItems.size() < 1) {
        return;
    }
    const XString& text = item.m_subItems[item.m_subIndex].m_text;
    int textWidth = font->MeasureTextWidth(text.c_str(), -1, -1, NULL);
    int spaceWidth = font->MeasureTextWidth(L" ", -1, -1, NULL);
    if (selected == index) {
        // hit areas of the arrows: the space between them is split in two halves
        int leftX = m_width - (rightWidth + leftWidth + textWidth + spaceWidth * 2);
        item.m_arrowX[0] = leftX - spaceWidth;
        item.m_arrowWidth[0] = leftWidth + spaceWidth * 2;
        short rightAreaX = m_width - (spaceWidth + rightWidth);
        short rightAreaWidth = rightWidth + spaceWidth * 2;
        item.m_arrowX[1] = item.m_arrowX[0] + (rightAreaX + rightAreaWidth - item.m_arrowX[0]) / 2;
        item.m_arrowWidth[1] = item.m_arrowX[1] - item.m_arrowX[0];
        item.m_arrowWidth[0] = item.m_arrowWidth[1];
        if (m_arrowLeft.GetSurface() != NULL) {
            int centerY = fontHeight / 2 + y;
            graphics.PushTransform();
            graphics.Translate(IntToFixed(m_width - rightWidth), IntToFixed(centerY - rightHalfHeight));
            graphics.Draw(*m_arrowRight.GetSurface(), NULL, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
            graphics.PopTransform();
            graphics.PushTransform();
            graphics.Translate(IntToFixed(leftX), IntToFixed(centerY - leftHalfHeight));
            graphics.Draw(*m_arrowLeft.GetSurface(), NULL, &WindowApp::m_instance->Color(), ICGraphics2d::Flip_None);
            graphics.PopTransform();
        }
    }
    DrawFontText(graphics, font, text.c_str(), m_width - (textWidth + rightWidth + spaceWidth), y, 0);
}

void SimpleDialog::ItemsWindow::Paint(ICGraphics2d& graphics)
{
    int count = m_items.size();
    if (count < 1) {
        return;
    }
    SimpleDialog* dialog = Dialog();
    int selected = m_selectedIndex;
    int iconWidth = 0;
    if (dialog->m_frameStyle != 1) {
        iconWidth = MaxIconWidth();
    }
    int textX = iconWidth;
    if (iconWidth > 0) {
        textX++;
    }
    CFont* font = CFontMgr::GetInstance()->GetFont(FONT_ITEM);
    int itemHeight = getItemHeight();
    int line = 0;
    for (int y = -m_scrollY; y < m_height; y += itemHeight + dialog->m_margins[MARGIN_ITEM_GAP]) {
        int top = 0;
        if (m_height >= VirtualHeight()) {
            top = m_topIndex;
        }
        int index = top + line;
        if (index >= count) {
            return;
        }
        Item& item = m_items[index];
        item.ResetLayout();
        if (item.m_type != ITEM_HIDDEN) {
            if (dialog->m_frameStyle == 0) {
                dialog->PaintItemIcon(graphics, index, index == selected, &item, iconWidth, itemHeight / 2 + y);
            }
            item.m_y = y;
            item.m_height = itemHeight + dialog->m_margins[MARGIN_ITEM_GAP];
            if (dialog->m_frameStyle == 1) {
                PaintItem(graphics, index, m_width / 2, y, itemHeight, true);
            } else {
                PaintItem(graphics, index, textX, y, itemHeight, false);
            }
            PaintSubItem(graphics, index, textX, y);
        }
        line++;
    }
}

void SimpleDialog::ItemsWindow::OnPointerEvent(Event& event)
{
    if (m_items.size() < 1) {
        return;
    }
    int dx;
    int dy;
    WindowApp::GetPointerOffset(event.m_param3, dx, dy);
    if (event.m_type == Event::POINTER_DRAGGED) {
        int scrollY = m_scrollY - dy;
        int virtualHeight = VirtualHeight();
        if (virtualHeight - scrollY < m_height) {
            scrollY = virtualHeight - m_height;
        }
        if (scrollY < 0) {
            scrollY = 0;
        }
        m_scrollY = scrollY;
        if (m_height < VirtualHeight()) {
            m_dragging = true;
        }
    } else if (event.m_type == Event::POINTER_PRESSED) {
        m_dragging = false;
    }
    if (m_dragging) {
        return;
    }
    int count = m_items.size();
    int selected = m_selectedIndex;
    int pointerX = event.m_param1;
    int pointerY = event.m_param2;
    int line = 0;
    for (int y = 0; y < m_height; line++) {
        int top = 0;
        if (m_height >= VirtualHeight()) {
            top = m_topIndex;
        }
        int index = top + line;
        if (index >= count) {
            return;
        }
        Item& item = m_items[index];
        int itemBottom = item.m_y + item.m_height;
        if (pointerY >= item.m_y && pointerY < itemBottom) {
            if (!item.m_enabled) {
                event.Clear();
                return;
            }
            if (index == selected) {
                // a press on one of the sub item arrows works like the left / right key
                for (int arrow = 0; arrow < 2; arrow++) {
                    if (pointerX >= item.m_arrowX[arrow] &&
                        pointerX < item.m_arrowX[arrow] + item.m_arrowWidth[arrow]) {
                        if (event.m_type == Event::POINTER_RELEASED) {
                            int command = COMMAND_RIGHT;
                            if (arrow == 0) {
                                command = COMMAND_LEFT;
                            }
                            // rebuild the event in place as a command (no copy of a temporary)
                            new (&event) Event(Event::COMMAND, command, 0, 0, 0);
                            OnCommand(event);
                        }
                        event.Clear();
                        return;
                    }
                }
            } else {
                SetSelectedIndex(index);
            }
            if (event.m_type == Event::POINTER_RELEASED) {
                int x = m_x + pointerX;
                if (x >= 0 && x < m_parent->m_width) {
                    SimpleDialog* dialog = Dialog();
                    if (item.m_subItems.size() < 1) {
                        dialog->ClickItem(item.m_id, true);
                    } else {
                        dialog->ClickSubitem(item.m_id, item.m_subItems[item.m_subIndex].m_id, true);
                    }
                }
            }
            event.Clear();
            return;
        }
        y = itemBottom - m_scrollY;
    }
}

void SimpleDialog::ItemsWindow::OnCommand(Event& event)
{
    int selected = m_selectedIndex;
    Item* item = &m_items[selected];
    if (item == NULL) {
        return;
    }
    int id = event.m_id;
    switch (id) {
    case COMMAND_UP:
    case COMMAND_DOWN: {
        // ?: (a select, not a branch) as in the original
        SetSelectedIndex(selected + (id == COMMAND_UP ? -1 : 1));
        UpdateScrollYByTopIndex();
        break;
    }
    case COMMAND_LEFT:
    case COMMAND_RIGHT: {
        // ?: (a select, not a branch) as in the original
        SetSubitemSelectedIndex(selected, item->m_subIndex + (id == COMMAND_LEFT ? -1 : 1));
        UpdateScrollYByTopIndex();
        break;
    }
    case COMMAND_SELECT:
        if (item->m_subItems.size() < 1) {
            Dialog()->ClickItem(item->m_id, false);
        } else {
            Dialog()->ClickSubitem(item->m_id, item->m_subItems[item->m_subIndex].m_id, false);
        }
        break;
    default:
        return;
    }
    event.Clear();
}

SimpleDialog::SimpleDialog(EDialogType type)
{
    FreeLayout();
    ClearAndSetFlags(FLAG_CHILDREN_FIRST, FLAG_CANCELABLE | FLAG_SCROLLBAR | FLAG_EVENTS_WHILE_SHOWING);
    SetAlign(0);
    m_frameStyle = 1;
    m_margins[MARGIN_SIDE] = 0x10;
    m_margins[MARGIN_TOP] = 0x14;
    m_margins[MARGIN_BOTTOM] = 0x1a;
    m_margins[MARGIN_ITEMS] = 0xa;
    m_margins[MARGIN_ITEM_GAP] = 0;
    m_margins[MARGIN_TITLE] = 0;
    m_type = type;
    m_titleFont = CFontMgr::GetInstance()->GetFont(FONT_TITLE);
    m_registerKeys = true;
    m_itemsWindow = NULL;
    m_textArea = new SimpleTextArea(CFontMgr::GetInstance()->GetFont(FONT_TEXT));
    m_textArea->SetRect(m_margins[MARGIN_SIDE], m_margins[MARGIN_TOP], m_width - m_margins[MARGIN_SIDE] * 2,
                        m_height - m_margins[MARGIN_TOP] - m_margins[MARGIN_BOTTOM], false);
    AddToFront(m_textArea);
}

SimpleDialog::~SimpleDialog()
{
}

void SimpleDialog::AdjustByRect(int x, int y, int width, int height)
{
    int right = x + width;
    int bottom = y + height;
    int pass = 1;
    bool again;
    do {
        int titleHeight = 0;
        int titleWidth = 0;
        if (m_title.ByteSize() != 0) {
            int titleGap = m_margins[MARGIN_TITLE];
            titleHeight = m_titleFont->GetFontHeight() + titleGap;
            titleWidth = m_titleFont->MeasureTextWidth(m_title.c_str(), -1, -1, NULL) + TITLE_EXTRA_WIDTH;
        }
        int textHeight = 0;
        int textAreaHeight = m_textArea->m_textHeight;
        if (textAreaHeight != 0) {
            textHeight = textAreaHeight;
            if (textAreaHeight < m_textArea->MinHeight()) {
                textHeight = m_textArea->MinHeight();
            }
        }
        int itemsHeight = 0;
        int itemsGap = 0;
        if (m_itemsWindow != NULL) {
            itemsHeight = m_itemsWindow->VirtualHeight();
            if (itemsHeight != 0 && textHeight != 0) {
                itemsGap = m_margins[MARGIN_ITEMS];
            }
        }
        int contentWidth = 0;
        if (textHeight != 0) {
            contentWidth = m_textArea->MinWidth() + m_margins[MARGIN_SIDE] * 2;
        }
        if (contentWidth < m_width) {
            contentWidth = m_width;
        }
        int itemsWidth = 0;
        if (m_itemsWindow != NULL) {
            itemsWidth = m_itemsWindow->VirtualWidth() + m_margins[MARGIN_SIDE] * 2;
        }
        if (contentWidth < itemsWidth) {
            contentWidth = itemsWidth;
        }
        int dialogWidth = titleWidth;
        if (dialogWidth < contentWidth) {
            dialogWidth = contentWidth;
        }
        int dialogX = m_x + m_width / 2 - dialogWidth / 2;
        if (dialogX < x) {
            dialogX = x;
        }
        int dialogHeight = titleHeight + textHeight + itemsHeight + itemsGap + m_margins[MARGIN_TOP] +
                           m_margins[MARGIN_BOTTOM];
        int dialogY = m_y + m_height / 2 - dialogHeight / 2;
        if (dialogY < y) {
            dialogY = y;
        }
        // a narrower dialog breaks its text into more lines: lay out again
        again = false;
        if (dialogX + dialogWidth > right) {
            dialogX = right - dialogWidth;
            if (dialogX < x) {
                dialogX = x;
            }
            if (dialogX + dialogWidth > right) {
                dialogWidth = right - dialogX;
                again = true;
            }
        }
        if (dialogY + dialogHeight > bottom) {
            dialogY = bottom - dialogHeight;
            if (dialogY < y) {
                dialogY = y;
            }
            if (dialogY + dialogHeight > bottom) {
                dialogHeight = bottom - dialogY;
                again = false;
            }
        }
        SetRect(dialogX, dialogY, dialogWidth, dialogHeight, true);
    } while (again && pass++ < MAX_ADJUST_PASSES);
}

void SimpleDialog::AdjustByDefRect()
{
    int width;
    int height;
    if (m_parent != NULL) {
        width = m_parent->m_width;
        height = m_parent->m_height;
    } else {
        width = WindowApp::m_instance->m_screenWidth;
        height = WindowApp::m_instance->m_screenHeight;
    }
    int margin = m_margins[MARGIN_SIDE];
    AdjustByRect(margin * 2, margin * 2, width - margin * 4, height - margin * 4);
}

void SimpleDialog::OnLayout()
{
    int titleHeight = 0;
    if (m_frameStyle == 1 && m_title.ByteSize() != 0) {
        int titleGap = m_margins[MARGIN_TITLE];
        titleHeight = m_titleFont->GetFontHeight() + titleGap;
    }
    if (m_itemsWindow != NULL) {
        int marginTop = m_margins[MARGIN_TOP];
        int itemsWidth = m_itemsWindow->VirtualWidth();
        int itemsHeight = m_itemsWindow->VirtualHeight();
        int textHeight = m_textArea->m_textHeight;
        int itemsY;
        if (textHeight != 0) {
            int minHeight = m_textArea->MinHeight();
            // ?: keeps the subtraction order of the original
            int available = m_height - titleHeight - m_margins[MARGIN_TOP] - m_margins[MARGIN_BOTTOM] -
                            (itemsHeight != 0 ? m_margins[MARGIN_ITEMS] + itemsHeight : 0);
            if (textHeight > available) {
                textHeight = available;
            }
            if (textHeight < minHeight) {
                textHeight = minHeight;
            }
            m_textArea->SetRect(m_margins[MARGIN_SIDE], m_margins[MARGIN_TOP] + titleHeight,
                                m_width - m_margins[MARGIN_SIDE] * 2, textHeight, true);
            itemsY = m_textArea->m_y + m_textArea->m_height + m_margins[MARGIN_ITEMS];
        } else {
            m_textArea->SetRect(m_margins[MARGIN_SIDE], m_margins[MARGIN_TOP] + titleHeight,
                                m_width - m_margins[MARGIN_SIDE] * 2, 0, true);
            itemsY = marginTop + titleHeight;
        }
        int itemsX = m_margins[MARGIN_SIDE];
        int maxWidth = m_width - itemsX * 2;
        if (itemsWidth > maxWidth) {
            itemsWidth = maxWidth;
        }
        int centeredX = (m_width - itemsWidth) / 2;
        if (centeredX >= itemsX) {
            itemsX = centeredX;
        }
        m_itemsWindow->SetRect(itemsX, itemsY, itemsWidth, m_height - itemsY - m_margins[MARGIN_BOTTOM], true);
    } else {
        m_textArea->SetRect(m_margins[MARGIN_SIDE], m_margins[MARGIN_TOP] + titleHeight,
                            m_width - m_margins[MARGIN_SIDE] * 2,
                            m_height - m_margins[MARGIN_TOP] - m_margins[MARGIN_BOTTOM], true);
    }
}

SimpleDialog::ItemsWindow* SimpleDialog::CreateItemInstance()
{
    return new ItemsWindow();
}

void SimpleDialog::AddItem(int id, const XString& text, int type)
{
    if (m_itemsWindow == NULL) {
        m_itemsWindow = CreateItemInstance();
        AddModal(m_itemsWindow);
    }
    m_itemsWindow->AddItem(id, text, NULL, type);
}

SimpleDialog::ItemFonts SimpleDialog::ItemFontsByIndex(int index) const
{
    return ItemFonts(CFontMgr::GetInstance()->GetFont(FONT_ITEM), CFontMgr::GetInstance()->GetFont(FONT_ITEM_PRESSED));
}

void SimpleDialog::SetText(const XString& text)
{
    m_textArea->SetText(text);
}

void SimpleDialog::SetTextFont(CFont* font)
{
    m_textArea->SetFont(font);
}

void SimpleDialog::SetTitle(const XString& title)
{
    m_title = title;
    HandleLayout();
}

void SimpleDialog::SetTitleFont(CFont* font)
{
    m_titleFont = font;
    HandleLayout();
}

void SimpleDialog::SetRect(int x, int y, int width, int height, bool force)
{
    Window::SetRect(x, y, width, height, force);
}

void SimpleDialog::OnEasyStarted()
{
}

void SimpleDialog::Init()
{
    CDH_SoftkeysScreen::Init();
}

void SimpleDialog::Shutdown()
{
}

void SimpleDialog::CancelDialog()
{
    ReverseEasy(COMMAND_CLOSE);
}

void SimpleDialog::PaintBackground(ICGraphics2d& graphics)
{
    if (m_type != DIALOG_TYPE_0 && m_type != DIALOG_TYPE_1) {
        return;
    }
    if (m_title.ByteSize() == 0) {
        DrawBorderFillRect(graphics, m_borderLeft.GetSurface(), m_cornerTopLeft.GetSurface(), m_borderTop.GetSurface(),
                           m_cornerTopRight.GetSurface(), m_borderRight.GetSurface(), m_cornerBottomRight.GetSurface(),
                           m_borderBottom.GetSurface(), m_cornerBottomLeft.GetSurface(), m_background.GetSurface(), 0, 0,
                           m_width, m_height);
    } else {
        // measured, but the frame is placed with fixed offsets below the title
        int titleWidth = m_titleFont->MeasureTextWidth(m_title.c_str(), -1, -1, NULL);
        int titleHeight = m_titleFont->GetFontHeight();
        DrawBorderFillRect(graphics, m_borderLeft.GetSurface(), m_cornerTopLeft.GetSurface(), m_borderTop.GetSurface(),
                           m_cornerTopRight.GetSurface(), m_borderRight.GetSurface(), m_cornerBottomRight.GetSurface(),
                           m_borderBottom.GetSurface(), m_cornerBottomLeft.GetSurface(), m_background.GetSurface(), 0,
                           TITLE_FRAME_TOP, m_width, m_height - TITLE_FRAME_TOP * 2);
    }
    if (m_title.ByteSize() != 0) {
        App::Inc2DLayer();
        DrawFontText(graphics, m_titleFont, m_title.c_str(), TITLE_X, TITLE_Y, ALIGN_LEFT | ALIGN_TOP);
        DrawLineImageHoriz(graphics, m_titleLineLeft.GetSurface(), m_titleLineMiddle.GetSurface(),
                           m_titleLineRight.GetSurface(), m_margins[MARGIN_SIDE],
                           m_titleFont->GetFontHeight() + TITLE_Y, m_width - m_margins[MARGIN_SIDE] * 2);
    }
}

void SimpleDialog::PaintScrollbar(ICGraphics2d& graphics)
{
    if ((m_flags & (FLAG_SCROLLBAR | FLAG_SHOW_AFTER_EASY | FLAG_CLOSE_AFTER_EASY)) != FLAG_SCROLLBAR) {
        return;
    }
    if (m_itemsWindow == NULL) {
        SimpleTextArea* text = m_textArea;
        int textX = text->m_x;
        int textY = text->m_y;
        int textWidth = text->m_width;
        int textHeight = text->m_height;
        DrawVertScrollbar(graphics, textX + textWidth + 1, textY, textHeight, m_scrollTop.GetSurface(),
                          m_scrollBottom.GetSurface(), m_scrollTrack.GetSurface(), m_scrollThumb.GetSurface(),
                          m_textArea->ScrollPos(), (float)m_textArea->m_height / (float)m_textArea->m_textHeight,
                          true);
    } else if (m_itemsWindow->m_height < m_itemsWindow->VirtualHeight()) {
        int x = m_textArea->m_x + m_textArea->m_width - 7;
        DrawVertScrollbar(graphics, x, m_itemsWindow->m_y, m_itemsWindow->m_height, m_scrollTop.GetSurface(),
                          m_scrollBottom.GetSurface(), m_scrollTrack.GetSurface(), m_scrollThumb.GetSurface(),
                          m_scrollThumbTop.GetSurface(), m_scrollThumbBottom.GetSurface(), m_itemsWindow->ScrollPos(),
                          (float)m_itemsWindow->m_height / (float)m_itemsWindow->VirtualHeight(), false);
    }
}

void SimpleDialog::Paint(ICGraphics2d& graphics)
{
    if (m_frameStyle == 1) {
        PaintBackground(graphics);
    }
    App::Inc2DLayer();
    PaintScrollbar(graphics);
}

void SimpleDialog::OnCommand(Event& event)
{
    if (event.m_id != COMMAND_BACK) {
        return;
    }
    if ((m_flags & FLAG_CANCELABLE) == 0) {
        return;
    }
    CancelDialog();
    event.Clear();
}

void SimpleDialog::OnGetFocus()
{
    if (!m_registerKeys) {
        return;
    }
    CUtility::RegisterUIKeys();
    CUtility::RegisterUISoftkeys();
}

void SimpleDialog::OnLostFocus()
{
    if (!m_registerKeys) {
        return;
    }
    CUtility::UnregisterUISoftkeys();
    CUtility::UnregisterUIKeys();
}
