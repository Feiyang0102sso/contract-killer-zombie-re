// BundleInterface.h - main bundle information (C++ facade over NSBundle).
// Owned by util/ios/BundleInterface.mm (b032, written by b027x).
#ifndef BUNDLEINTERFACE_H
#define BUNDLEINTERFACE_H

#include "CStrWChar.h"

class BundleInterface {
public:
    /** CFBundleIdentifier of the main bundle (left unchanged without a main bundle). */
    static void BundleIdentifierString(com::glu::platform::components::CStrWChar& identifier);
    /** CFBundleVersion of the main bundle (left unchanged without a main bundle). */
    static void BundleVersionString(com::glu::platform::components::CStrWChar& version);
};

#endif
