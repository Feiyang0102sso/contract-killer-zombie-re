// PROVISIONAL (created by b048): owner src/bounty_hunter/CLoadingScreen.cpp not written yet
// CLoadingScreen.h - loading screen window; only what src/dh3d uses is declared.
// b074i: complete owner restored; the original PROVISIONAL/guessed-name comments are kept as history.
#ifndef CLOADINGSCREEN_H
#define CLOADINGSCREEN_H

#include "WindowBase.h"
#include "xString.h"
#include "Vector_ss.h"
class CDH_LoadingBarWidget;
class TextWindow;
class CTip;

class CLoadingScreen : public Window {
public:
    /** One loading task; Load() runs once per frame until it returns true. */  // added by b055x
    class IStep {  // added by b055x
    public:  // added by b055x
        virtual ~IStep() {}  // added by b055x
        virtual bool Load(CLoadingScreen* screen) = 0;  // added by b055x
        virtual int GetTotal() const { return 1; }  // added by b055x
        virtual int GetStep() const { return 0; }  // added by b055x
        virtual bool IsDestructible() const { return true; }  // added by b055x
    };  // added by b055x

    /** Queue a loading step (`name` for the debug output). */  // added by b055x
    void AddLoadingStep(IStep* step, const char* name);  // added by b055x
    /** Progress range of the bar. */  // added by b055x
    void SetRange(unsigned long first, unsigned long last, bool flag);  // added by b055x

    CLoadingScreen(bool showTips);
    virtual ~CLoadingScreen();
    virtual void Init();
    virtual void Update();
    virtual void OnPointerPressed(Event& event);
    virtual int ShouldBeModal() { return 1; } // original 11d7a0, returns through the real Window int slot
    /** First CLoadingScreen virtual (vtable slot 0xbc); argument is a hashed id (name guessed). */
    // Historical guessed name SetMusicId: the real symbol of original 11d4cc is SetFinishCmd, which writes +fc.
    virtual void SetFinishCmd(unsigned int id);

    void ClearSteps();
    void UpdateSteps();

    void SetInfoWindow(Window* window);
    /** Background image resource. */
    void SetBackground(const char* name)
    {
        m_background = XString(name);
    }

    // historical unsigned char m_dataac[0x102 - 0xac]; layout not known yet, now the real dual Vector.
    Vector<IStep*> m_steps;                // 0xac, tasks are deleted depending on IsDestructible
    Vector<const char*> m_stepNames;       // 0xbc, borrowed names, not freed
    CDH_LoadingBarWidget* m_loadingBar;    // 0xcc, owned by the Window tree
    Window* m_infoWindow;                 // 0xd0
    TextWindow* m_tipWindow;              // 0xd4
    CTip* m_tip;                          // 0xd8, owned by the manager
    bool m_initialized;                  // 0xdc
    unsigned long m_rangeFirst;           // 0xe0
    unsigned long m_rangeLast;            // 0xe4
    unsigned int m_currentStep;           // 0xe8
    unsigned int m_total;                 // 0xec, not written by the constructor
    unsigned int m_completed;             // 0xf0
    bool m_stepFinished;                  // 0xf4
    float m_remaining;                    // 0xf8
    unsigned int m_finishCommand;         // 0xfc
    bool m_finished;                      // 0x100
    bool m_waitForTap;                    // 0x101, historical showTips parameter name
    bool m_field102;                        // 0x102 cleared by CGPSMapScreen::OnCreateState
    XString m_background;                   // 0x104
};

#endif
