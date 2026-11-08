// k: original 150a2c complete 52B. f0/f4 only prove member access on this existing instance; total size not guessed.
#include "CGameplayHPBar.h"
#include "SGImageWindow.h"

void CGameplayHPBar::SetPercent(float percent)
{
    if (!(percent >= 0.0f)) {
        return;
    }
    if (!(percent <= 1.0f)) {
        return;
    }
    unsigned char* object = reinterpret_cast<unsigned char*>(this);
    float* percentField = reinterpret_cast<float*>(object + 0xf0);
    SGClippedImageWindow** fillField = reinterpret_cast<SGClippedImageWindow**>(object + 0xf4);
    *percentField = percent;
    (*fillField)->SetClipping(percent, 1.0f);
}
