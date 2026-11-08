// j: original 159c00 ctor ignores the message parameter; uninitialized fields kept as in the original.
#ifndef CTARGETTEDNEWSWINDOW_H
#define CTARGETTEDNEWSWINDOW_H
#include "WindowBase.h"
struct STargettedMessage;
struct SMessage;

class CTargettedNewsWindow : public Window {
public:
    CTargettedNewsWindow(const STargettedMessage* message);
    virtual ~CTargettedNewsWindow() {}
    virtual void OnCommand(Event& event);
    void HandleRevengePressed();
    int m_sender;              // ac, not initialized by this constructor.
    bool m_flagb0;             // b0
    int m_fieldb4;             // b4
    const SMessage* m_message; // b8, not initialized by this constructor.
    int m_messageId;           // bc, not initialized by this constructor.
};
#endif
