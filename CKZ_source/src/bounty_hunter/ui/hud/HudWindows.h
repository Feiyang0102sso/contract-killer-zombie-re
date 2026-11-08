// k: original 152d30..1532ec complete Hud helper windows; real allocation sizes see GameplayHUD.
#ifndef HUDWINDOWS_H
#define HUDWINDOWS_H
#include "BaseDialog.h"
#include "SGImageWindow.h"

class HudWindow : public Window {
public:
    HudWindow() : Window(false) {}
    bool CheckCollision(int x, int y, float factor);
};

class ProgressFillHudImageWindow : public ImageWindow {
public:
    ProgressFillHudImageWindow() : m_progress(1.0f), m_isAnimating(false)
    {
        m_blendGreen = 1.0f;
        m_blendBlue = 1.0f;
        m_blendAlpha = 1.0f;
        m_timeLeft = 0;
        m_duration = 600;
    }
    virtual ~ProgressFillHudImageWindow() {} // Original 153280 only calls ImageWindow.
    void SetIsAnimating(bool enabled);
    virtual void Update();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics);
    float m_progress;    // d8, actual clip ratio.
    float m_blendGreen;  // dc
    float m_blendBlue;   // e0
    float m_blendAlpha;  // e4
    bool m_isAnimating;  // e8
    int m_timeLeft;      // ec
    int m_duration;      // f0, total f4.
};

class ConsumableHudImageWindow : public HudWindow {
public:
    ConsumableHudImageWindow(const char* image);
    virtual ~ConsumableHudImageWindow() {} // Original 1532c8 only calls Window; child windows belong to the tree.
    void SetCount(int count);
    ProgressFillHudImageWindow* m_fill; // ac
    TextWindow* m_countText;            // b0
    int m_count;                       // b4, total b8.
};

class FlashingEffectWindow : public SGImageWindow {
public:
    FlashingEffectWindow();
    virtual ~FlashingEffectWindow() {} // Original 1532a4 calls SGImageWindow.
    void StartFlashing(int period);
    virtual void Update();
    int m_elapsed;           // 10c
    bool m_active;           // 110
    bool m_stopRequested;    // 111
    int m_period;            // 114, total 118.
};
#endif
