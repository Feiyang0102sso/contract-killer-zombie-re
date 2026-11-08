// TapjoyUtil.mm - platform helpers for the Tapjoy integration.
#include "TapjoyUtil.h"

#include "CApplet.h"
#include "ICCore.h"

int TapjoyUtil::GetDeviceOrientation()
{
    return CApplet::m_pCore->GetDeviceOrientation();
}
