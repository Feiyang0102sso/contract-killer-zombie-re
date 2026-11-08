// u543 full constructor; the three SScreen argument pairs are each evaluated independently, keeping the original query count.
#include "CQuestZombieDialogWindow.h"
#include "SGImageWindow.h"
#include "SScreenAdaptingConstant.h"

CQuestZombieDialogWindow::CQuestZombieDialogWindow(const CQuestManager::SDialog& dialog)
    : CZombieDialogWindow(2)
{
    m_messageContainer->SetAlign(12);
    Window* message = m_messageContainer;
    int top = SScreenAdaptingConstant(50)();
    int left = SScreenAdaptingConstant(90)();
    message->SetOutsetSpacing(top, 0, 0, left);
    m_messageContainer->SetPercentWidth(85, 0, 0);
    ImageWindow* image = new ImageWindow;
    image->SetImageById(dialog.m_image);
    image->SetLayoutType(LAYOUT_SIMPLE);
    image->SetAlign(33);
    top = SScreenAdaptingConstant(15)();
    left = SScreenAdaptingConstant(42)();
    image->SetOutsetSpacing(top, 0, 0, left);
    m_background->AddToFront(image);
    SetTitle(dialog.m_title);
    SetText(dialog.m_text, false, NULL);
    AddButton(BUTTON_OK, (unsigned int)-1, 0, 0, true, 20);
}
