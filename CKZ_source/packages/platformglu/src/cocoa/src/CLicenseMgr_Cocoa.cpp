// CLicenseMgr_Cocoa.cpp - iOS license manager (all queries answer "none").
#include "CLicenseMgr_Cocoa.h"

using namespace com::glu::platform::components;

ICLicenseMgr* ICLicenseMgr::CreateInstance()
{
    return new CLicenseMgr_Cocoa;
}

CLicenseMgr_Cocoa::~CLicenseMgr_Cocoa()
{
}

void CLicenseMgr_Cocoa::ReadLicenseInfo()
{
}

int CLicenseMgr_Cocoa::GetLicenseType()
{
    return 0;
}

int CLicenseMgr_Cocoa::GetRemainingTimeSeconds()
{
    return 0;
}

int CLicenseMgr_Cocoa::GetRemainingUsageCount()
{
    return 0;
}

unsigned char CLicenseMgr_Cocoa::RequestActivation()
{
    return 0;
}

unsigned char CLicenseMgr_Cocoa::IsActivating()
{
    return 0;
}
