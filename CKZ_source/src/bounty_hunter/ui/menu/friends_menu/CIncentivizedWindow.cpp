// j: the only method at original 159654; no evidence yet for the complete class/base class, so no allocatable size is faked.
#include "CIncentivizedWindow.h"

Vector<int> CIncentivizedWindow::GetSelectedIds() const
{
    // Original: hidden sret in r0, this in r1; copies the Vector<int> actually at this+168.
    // Only a real partial view of an existing instance is provided here; the unknown leading member types are not declared based on it.
    const Vector<int>* ids = reinterpret_cast<const Vector<int>*>(
        reinterpret_cast<const unsigned char*>(this) + 0x168);
    return *ids;
}
