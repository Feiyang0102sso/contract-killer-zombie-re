// CLicenseMgr_Cocoa.h - iOS license manager: nothing to read, no license type, never activating.
#ifndef CLICENSEMGR_COCOA_H
#define CLICENSEMGR_COCOA_H

#include "ICLicenseMgr.h"

class CLicenseMgr_Cocoa : public com::glu::platform::components::ICLicenseMgr {
public:
    virtual ~CLicenseMgr_Cocoa();

    virtual void ReadLicenseInfo();
    virtual int GetLicenseType();
    virtual int GetRemainingTimeSeconds();
    virtual int GetRemainingUsageCount();
    virtual unsigned char RequestActivation();
    virtual unsigned char IsActivating();
};

#endif
