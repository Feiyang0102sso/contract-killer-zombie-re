// CAdManager_AdMarvel_Cocoa.cpp - plain C++ part of the AdMarvel wrapper (the rest lives in
// CAdManager_AdMarvel_Cocoa_mm.mm because it talks to Objective-C).
#include "CAdManager_AdMarvel_Cocoa.h"

void CAdManager_AdMarvel_Cocoa::HandleRender()
{
    // AdMarvel draws through UIKit, nothing to render here
}

CAdManager_AdMarvel_Cocoa::~CAdManager_AdMarvel_Cocoa()
{
    Destroy();
}

CAdManager_AdMarvel_Cocoa::CAdManager_AdMarvel_Cocoa()
    : CSingleton(ClassId)
{
}
