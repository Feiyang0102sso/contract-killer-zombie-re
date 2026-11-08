// j: keeps the original spelling Recieved; original Window-derived bc; item/message are both non-owning.
#ifndef CGIFTRECIEVEDNEWSWINDOW_H
#define CGIFTRECIEVEDNEWSWINDOW_H
#include "WindowBase.h"
struct SGiftReceivedMessage;
class CItem;
class CGiftRecievedNewsWindow : public Window {
public:
    CGiftRecievedNewsWindow(const SGiftReceivedMessage* message);
    virtual ~CGiftRecievedNewsWindow() {}
    void CreateUserInterface(const XString& friendName, const XString& itemName, bool fromFriend);
    virtual void OnCommand(Event& event);
    void HandleGiftAccepted();
    CItem* m_item; // ac
    int m_fieldb0;
    int m_messageId;
    const SGiftReceivedMessage* m_message;
};
#endif
