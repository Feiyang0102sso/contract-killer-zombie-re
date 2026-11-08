// PROVISIONAL (created by b057x): owner src/CNode.cpp not written yet
// CNode.h - a node of the application's node tree (start/stop, events, update, render).
// Slot order from the vtable of CNode; fields from CNode::CNode() and its accessors.
// Restored by b063: preceding PROVISIONAL records the earlier partial declaration.
#ifndef CNODE_H
#define CNODE_H

#include "ICDisplayProgram.h"

class CNode {
public:
    CNode();
    virtual ~CNode();

    virtual int InitGraphicsOptimizations(com::glu::platform::graphics::ICDisplayProgram::Mode mode,
                                          unsigned short a, unsigned int b, unsigned short c,
                                          unsigned short d, unsigned short e);
    virtual void DestroyGraphicsOptimizations();
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
    virtual void RenderBegin();
    virtual void RenderEnd();
    virtual void SyncExternalSystemsWithRenderBegin();

    unsigned int m_nodeId;          // 0x04 0x02642625
    int m_interrupt;                // 0x08
    com::glu::platform::graphics::ICDisplayProgram* m_displayProgram; // 0x0c graphics optimization program
    unsigned char m_data10[0x30 - 0x10];
};

#endif
