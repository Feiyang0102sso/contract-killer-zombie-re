// Application node defaults and rendering synchronization, from 0x0e1428..0x0e14ee.
#include "CNode.h"
#include "ICGraphics.h"
#include "ICGraphics2d.h"

using namespace com::glu::platform::graphics;

CNode::CNode()
{
    m_nodeId = 0x02642625;
    m_interrupt = 0;
    // The original leaves the graphics program and the trailing fields untouched.
}

CNode::~CNode()
{
    // Original D1/D2 are return-only; graphics teardown is explicitly separate.
}

int CNode::InitGraphicsOptimizations(ICDisplayProgram::Mode mode, unsigned short a, unsigned int b,
                                    unsigned short c, unsigned short d, unsigned short e)
{
    return 0; // Original 0x0e145c: movs r0,#0; bx lr.
}

void CNode::DestroyGraphicsOptimizations()
{
    if (m_displayProgram != NULL) {
        delete m_displayProgram;
        m_displayProgram = NULL;
    }
}

int CNode::Start() { return 0; } // Original 0x0e147c.
void CNode::Stop() {} // Original 0x0e1480: return-only hook.
void CNode::Activate() {} // Original 0x0e1484: return-only hook.
void CNode::Deactivate() {} // Original 0x0e1488: return-only hook.
int CNode::HandleEvent(unsigned int eventId, unsigned int param1, unsigned int param2) { return 0; } // 0x0e148c.
int CNode::HandleUpdate(int elapsed) { return 0; } // Original 0x0e1490.
int CNode::HandleRender() { return 0; } // Original 0x0e1494.

void CNode::SetInterrupt(int interrupt)
{
    if (m_interrupt == 0) {
        m_interrupt = interrupt;
    }
}
int CNode::GetInterrupt() { return m_interrupt; }
void CNode::ClearInterrupt() { m_interrupt = 0; }

void CNode::RenderBegin()
{
    ICGraphics* graphics = ICGraphics::GetInstance();
    ICGraphics2d::GetInstance();
    graphics->GetRasterizerState();
    graphics->Sync();
    SyncExternalSystemsWithRenderBegin();
}
void CNode::RenderEnd()
{
    ICGraphics::GetInstance();
    ICGraphics2d::GetInstance();
}
void CNode::SyncExternalSystemsWithRenderBegin()
{
    // Original 0x0e14ec: return-only virtual hook.
}
