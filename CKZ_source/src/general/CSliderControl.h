// PROVISIONAL (created by b057x): CSliderControl is declared for CSliderControl.cpp only; the
// rest of the class (inline code used by CGameplayHUD.cpp) is not known yet.
// CSliderControl.h - slider window of the HUD.
#ifndef CSLIDERCONTROL_H
#define CSLIDERCONTROL_H

#include "WindowBase.h"

class CSliderControl : public Window {
public:
    /** Enable or disable the slider (input and drawing flags). */
    void Enable(bool enable);
    /** original HUD15140c reads +100 of an existing instance; no allocation evidence found, so the full size is not guessed from this. */
    float GetValue() const
    {
        return *reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(this) + 0x100);
    }
};

#endif
