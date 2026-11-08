// CLoginSteps.h - login steps of the game's gServe login flow. (created by b057x, completed by b047x)
#ifndef CLOGINSTEPS_H
#define CLOGINSTEPS_H

#include "CNGSLoginFlow.h"

/** Checks the game version with the server (CNGSLoginStep_CheckVersion does the request). */
class VersionCheckStep : public CNGSLoginStep_CheckVersion {
public:
    /** Event 7 (version invalid) shows the update prompt, anything else continues the flow. */
    virtual void OnEvent(eLoginFlowEvent event, const char* caller);
};

#endif
