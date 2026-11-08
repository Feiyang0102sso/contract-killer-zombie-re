// CSliderControl.cpp - slider window of the HUD.
#include "CSliderControl.h"

void CSliderControl::Enable(bool enable)
{
    if (enable) {
        SetFlags(FLAG_VISIBLE);
        SetFlags(FLAG_ENABLED);
        SetFlags(FLAG_UPDATE);
    } else {
        ClearFlags(FLAG_VISIBLE);
        ClearFlags(FLAG_ENABLED);
        ClearFlags(FLAG_UPDATE);
    }
}
