// The only method kept by original u476; the header still declares only the proven field view and does not guess the full class size/constructor.
#include "CUnitInfoScreen.h"
#include "CStdUtil_Cocoa.h"

void CUnitInfoScreen::FlushBeforeCloseTimer()
{
    int* timer = reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(this) + 0x174);
    *timer = CStdUtil_Cocoa::GetUpTimeMS(); // Original 122646 calls the utility directly, not through WindowApp.
}
