// PROVISIONAL (created by b057x): owner src/CUtility.cpp not written yet
// CUtility.h - helpers of the game; only what src/general uses.
// Restored by b063; the preceding PROVISIONAL is historical.
#ifndef CUTILITY_H
#define CUTILITY_H

#include "CStrWChar.h"  // added by b048

class CUtility {
public:
    /** Register the key mappings of the user interface. */
    static void RegisterUIKeys();
    /** Register the soft keys of the user interface. */  // added by b058
    static void RegisterUISoftkeys();  // added by b058
    static void UnregisterUISoftkeys();  // added by b058
    static void UnregisterUIKeys();  // added by b058
    /** Localized text of string id `id`. */  // added by b048
    static com::glu::platform::components::CStrWChar GetString(const char* id);  // added by b048
    /** Localized text of string id `id` into `text`. */  // added by b055x
    static void GetString(com::glu::platform::components::CStrWChar& text, const char* id);  // added by b055x
    static void AppendString(com::glu::platform::components::CStrWChar& text, const char* id);
};

#endif
