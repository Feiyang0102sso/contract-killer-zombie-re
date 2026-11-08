// b081j: original 159fac..15a330 complete wait window; Window owns the subtree, dot is owned by this class.
#ifndef CWAITWINDOW_H
#define CWAITWINDOW_H
#include "WindowBase.h"

class CWaitWindow : public Window {
public:
    struct CDot {
        void Update(int elapsedMS);
        int m_unresolved00;
        int m_unresolved04;
        int m_radius;                   // 0x08
        int m_unresolved0c;
        int m_durationMS;               // 0x10
        int m_delayMS;                  // 0x14, not read by the original Update
        unsigned int m_elapsedMS;       // 0x18
        float m_percent;                // 0x1c, not written by the original constructor
    };
    CWaitWindow();
    virtual ~CWaitWindow();
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    Vector<CDot*> m_dots;               // 0xac
    int m_angleStep;                    // 0xbc
    unsigned int m_fillColor;           // 0xc0, total 0xc4
};
#endif
