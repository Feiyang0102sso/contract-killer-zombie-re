// j: original 159664 complete LevelUp, Window-derived d4; World is destroyed before Animations.
#ifndef CLEVELUPWINDOW_H
#define CLEVELUPWINDOW_H
#include "WindowBase.h"
#include "SwerveHelper.h"
class CLevelUpWindow : public Window {
public:
    CLevelUpWindow();
    virtual inline ~CLevelUpWindow() {}
    void NextState();
    virtual void Update();
    virtual void Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics);
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    SwerveAnimations m_animations; // ac
    SwerveAnimations::Animation* m_currentAnimation; // c0
    swerve::World<CSwerveAssertingErrorHandler> m_world; // c4
    int m_fieldc8;                // Not written by the original ctor.
    unsigned int m_remainingMS;   // cc
    int m_state;                  // d0
};
#endif
