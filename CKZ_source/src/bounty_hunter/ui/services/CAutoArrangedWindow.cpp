// b081j: the grid is decided by the size of the last item; keeps the original upper-limit comparison and the tree side effects after failure.
#include "CAutoArrangedWindow.h"
#include "App.h"

CAutoArrangedWindow::CAutoArrangedWindow(int width, int height)
    : Window(false), m_singleRow(false), m_maxRows(0), m_maxColumns(0)
{
    SetLayoutType(LAYOUT_NONE);
    if (App::IsAtrix()) {
        width = 750;
    }
    SetSize(width, height);
}

CAutoArrangedWindow::~CAutoArrangedWindow() {}

bool CAutoArrangedWindow::UpdateContentLayout()
{
    int count = m_windows.m_size;
    if (count == 0) {
        return true;
    }
    int width = m_width;
    Window* last = m_windows.m_items[count - 1];
    int itemWidth = last->m_width;
    int itemHeight = last->m_height;
    int columns = width / itemWidth;
    if (m_maxColumns != 0 && columns > m_maxColumns) {
        columns = m_maxColumns;
    }
    bool singleRow = m_singleRow;
    int rows = 1;
    if (!singleRow) {
        rows = m_height / itemHeight;
    }
    if (m_maxRows != 0 && rows > m_maxRows) {
        rows = m_maxRows;
    }
    if (rows < 1 || columns < 1 || rows * columns < count) {
        return false;
    }
    int spacingX = (width - columns * itemWidth) / (columns + 1);
    int spacingY = 0;
    if (!singleRow) {
        spacingY = (m_height - rows * itemHeight) / (rows + 1);
    }
    for (int index = 0; index < m_windows.m_size; ++index) {
        if (IsChildExisting(m_windows.m_items[index]) == 1) {
            Remove(m_windows.m_items[index]);
        }
    }
    int x = spacingX;
    int y = spacingY;
    int column = 0;
    int row = 0;
    for (int index = 0; index < m_windows.m_size; ++index) {
        Window* window = m_windows.m_items[index];
        if (m_maxRows != 0 && row >= m_maxRows) {
            return false;
        }
        // Original 159ee2 is bgt; do not change it to the seemingly symmetric >=.
        if (m_maxColumns != 0 && column > m_maxColumns) {
            return false;
        }
        AddToFront(window);
        ++column;
        window->SetLayoutType(LAYOUT_NONE);
        window->SetCorner(x, y);
        if (column == columns) {
            ++row;
            y += itemHeight + spacingY;
            column = 0;
            x = spacingX;
        } else {
            x += itemWidth + spacingX;
        }
    }
    return true;
}

bool CAutoArrangedWindow::Add(Window* window)
{
    m_windows.addElement(window);
    if (UpdateContentLayout()) {
        return true;
    }
    m_windows.removeElementAt(m_windows.m_size - 1);
    return false;
}
