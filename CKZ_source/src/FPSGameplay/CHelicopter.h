// CHelicopter - the action queue and two animations inside the original CSwerveGame.cpp, original allocation 0x30.
#ifndef CHELICOPTER_H
#define CHELICOPTER_H
#include "tws.h"
#include "xString.h"
#include "CStrChar.h"
#include "CVector.h"
class CDH_Animation;
class CHelicopter {
public:
    typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
    typedef swerve::World<CSwerveAssertingErrorHandler> World;
    struct SAction {
        // Original aggregate type; declares no constructor, keeping the whole-block zeroing before the final clang placement copy.
        // Implicit copy/assignment/destructor call the real owning operations of CStrChar; must not copy 40B of raw memory.
        int m_sound;
        com::glu::platform::components::CStrChar m_event;
        bool m_bodyLoop;
        int m_bodyController;
        bool m_positionLoop;
        int m_positionController;
        int m_elapsed;
    };
    CHelicopter(World world, XString resource);
    ~CHelicopter();
    void Update(int elapsedMs);
    bool IsLanded();
    bool IsFlying();
    void Land();
    void ForceLand();
    void Takeoff();
    void EngineOn();
    World m_world; // 0
    Group m_original; // 4
    Group m_model; // 8
    CDH_Animation* m_bodyAnimation; // c
    CDH_Animation* m_positionAnimation; // 10
    com::glu::platform::core::CVector<SAction> m_actions; // 14
    bool m_landed; // 2c
    bool m_flying;
    bool m_engineOn;
};
#endif

// Keeps the historical comments of the original thin declaration.
#if 0
// PROVISIONAL: real methods owned by CSwerveGame.cpp; no allocation or layout claim.
#ifndef CHELICOPTER_H
#define CHELICOPTER_H
class CHelicopter {
public:
    bool IsFlying(); // 0x100fb8
    bool IsLanded(); // 0x100f80
    void ForceLand(); // 0x10111c
    void Land(); // 0x100fcc
    void Takeoff(); // 0x1011e0
    void EngineOn(); // 0x101378
};
#endif

#endif

