// b074i: original 1484cc real Window inheritance / original Map allocates cc; only external declarations and the proven size.
#ifndef CEFFECTSWINDOW_H
#define CEFFECTSWINDOW_H
#include "WindowBase.h"
#include "Vector_ss.h"
class SGImageWindow;
class CEffectsWindow : public Window {
public:
    // The historical four-parameter names width/height/parameter/fourth have been corrected per the store order at 1484ea.
    CEffectsWindow(int archetype, int character, int width, int height);
    virtual inline ~CEffectsWindow() {} // The Vector only owns the records; the Window base class destroys the child windows afterwards.
    virtual void Update();
    void CreateEffect(int animation, int periodMS, int x, int y, bool randomPosition);
    struct SEffect {
        SGImageWindow* m_window;
        int m_x;
        int m_y;
        int m_periodMS;
        int m_remainingMS;
        bool m_randomPosition;
    };
    void StartEffect(SEffect& effect);
    // Historical unsigned char m_unresolvedAc[0xcc - 0xac], now converted to the real layout.
    int m_effectWidth;       // ac
    int m_effectHeight;      // b0
    int m_archetype;         // b4
    int m_character;         // b8
    Vector<SEffect> m_effects; // bc; the window is owned by the Window subtree.
};
#endif
