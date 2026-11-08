// CFpsCursor.h - cursor of the shooting game: tap to shoot, sight window, reloading message.
#ifndef CFPSCURSOR_H
#define CFPSCURSOR_H

#include "DGCursor.h"
#include "WindowBase.h"

class SG_Presenter;

class CFPSCursor : public DGCursor {
public:
    /** Cross hair of the sniper sight: four bars around the center and a center dot. */
    class SightWindow : public Window {
    public:
        SightWindow()
            : Window(false)
        {
            m_thickness = 2;
            m_length = 10;
        }

        virtual ~SightWindow()
        {
        }

        virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);

        int m_thickness;    // 0xac bar width
        int m_length;       // 0xb0 bar length (also the gap, set from the zoom)
    };

    CFPSCursor();
    virtual ~CFPSCursor();
    virtual int GetAimX() const
    {
        return m_aimX;
    }
    virtual int GetAimY() const
    {
        return m_aimY;
    }
    virtual void ShowCross(bool show);
    virtual void onWeaponSwitch();
    virtual void onFire();
    virtual void onPointerMove(int x, int y, int deltaX, int deltaY);
    virtual void onPointerBreak();
    virtual void onPointerPressed(int x, int y);
    virtual void onPointerReleased();
    virtual void DrawImpl();
    virtual void UpdateImpl(int elapsedMs);
    virtual void InitImpl();
    virtual void DeInitImpl();
    virtual void EnableSight(bool enable);
    virtual void ShowSight(bool show);

    /** Shoot at the aim position with the active weapon when it is ready and loaded. */
    void HandleShot();
    /** "Reloading" text at the aim position while the active weapon reloads. */
    void DrawReloadingMessage();
    void CreateSightWindow();

    int m_aimX;                     // 0x54 screen center
    int m_aimY;                     // 0x58
    SG_Presenter* m_shotPresenter;  // 0x5c
    int m_pressX;                   // 0x60
    int m_pressY;                   // 0x64
    float m_pressCamera1c;          // 0x68 camera field 0x1c when pressed
    float m_pressCamera0c;          // 0x6c camera field 0x0c when pressed
    bool m_pressed;                 // 0x70
    bool m_moved;                   // 0x71 moved since pressed
    bool m_shootRequested;          // 0x72
    bool m_shotAnimating;           // 0x73
    bool m_showCross;               // 0x74
    SightWindow* m_sightWindow;     // 0x78
};

#endif
