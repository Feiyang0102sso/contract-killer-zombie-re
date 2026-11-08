// j: original 157708..157bca, Window-derived, total b8; three members are initialized in CreateUserInterface.
#ifndef CNEWSFEEDWINDOW_H
#define CNEWSFEEDWINDOW_H
#include "WindowBase.h"
struct SGiftReceivedMessage;
struct SServerAwardMessage;
struct SFriendJoinedGameMessage;
struct STargettedMessage;
struct SMissionInviteMessage;
class CNewsFeedWindow : public Window {
public:
    CNewsFeedWindow();
    virtual ~CNewsFeedWindow();
    void CreateUserInterface();
    void ReceiveGift(const SGiftReceivedMessage* message);
    void ReceiveServerAward(const SServerAwardMessage* message);
    void FriendInstall(const SFriendJoinedGameMessage* message);
    void Targetted(const STargettedMessage* message);
    void MissionInvite(const SMissionInviteMessage* message);
    Window* m_content;
    int m_column;
    int m_row;
};
#endif
