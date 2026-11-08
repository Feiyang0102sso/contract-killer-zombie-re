// j: original 15996c complete mission invite; in the original, accept/ignore both only delete the message and close.
#ifndef CMISSIONINVITENEWSWINDOW_H
#define CMISSIONINVITENEWSWINDOW_H
#include "WindowBase.h"
struct SMissionInviteMessage;
class CMissionInviteNewsWindow : public Window {
public:
    CMissionInviteNewsWindow(const SMissionInviteMessage* message);
    virtual ~CMissionInviteNewsWindow() {}
    virtual void OnCommand(Event& event);
    void CreateUserInterface();
    int m_senderId;
    int m_missionId;
    int m_fieldb4;
    const SMissionInviteMessage* m_message;
};
#endif
