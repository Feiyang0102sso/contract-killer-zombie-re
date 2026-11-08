// PROVISIONAL: original u476 keeps only FlushBeforeCloseTimer; this header only serves Game's existing pointer accesses and does not declare the full allocation size.
#ifndef CUNITINFOSCREEN_H
#define CUNITINFOSCREEN_H
#include "WindowBase.h"
class CUnit;

class CUnitInfoScreen : public Window {
public:
    void FlushBeforeCloseTimer(); // Original 122640, writes the GetUpTimeMS result to +174.
    CUnit* GetUnit() const
    {
        // Game original ff4cc reads +b0; it then calls Window::Close directly on that window, the base class address is the same.
        const unsigned char* address = reinterpret_cast<const unsigned char*>(this);
        return *reinterpret_cast<CUnit* const*>(address + 0xb0);
    }
    int GetStateValue() const
    {
        const unsigned char* address = reinterpret_cast<const unsigned char*>(this);
        return *reinterpret_cast<const int*>(address + 0x178); // Read at original ff4c4, compared with 3 at ff4c8.
    }
};
#endif
