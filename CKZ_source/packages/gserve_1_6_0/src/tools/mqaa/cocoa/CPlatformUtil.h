// Owner: tools/mqaa/cocoa/CPlatformUtil.mm (b028x; started as a PROVISIONAL)
// CPlatformUtil.h - device information for the gServe message layer.
#ifndef CPLATFORMUTIL_H
#define CPLATFORMUTIL_H

#include "CStrWChar.h"

class CPlatformUtil {
public:
    /** Name the user gave the device (UIDevice name). */
    static com::glu::platform::components::CStrWChar getDeviceName();
};

#endif
