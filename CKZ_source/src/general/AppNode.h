// AppNode.h - node of the application node tree that hosts the game application (App) and
// forwards the platform events, updates and rendering to it.
#ifndef APPNODE_H
#define APPNODE_H

#include "CNode.h"

class App;

class AppNode : public CNode {
public:
    AppNode();
    virtual ~AppNode();

    virtual int Start();
    virtual void Stop();
    virtual void Activate();
    virtual void Deactivate();
    virtual int HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2);
    virtual int HandleUpdate(int elapsed);
    virtual int HandleRender();
    virtual void SetInterrupt(int interrupt);
    virtual int GetInterrupt();
    virtual void ClearInterrupt();

    App* m_app;     // 0x30 created by Start(), destroyed by Stop()
};

#endif
