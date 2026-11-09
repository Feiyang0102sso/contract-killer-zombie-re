// VIPWindow.cpp - the VIP offer window.
//
// Every function of this unit was dead-stripped from the shipped binary (nothing created the
// window); only the static initialization of the unit survives. It computes the class id of
// ICInAppPurchase (header static, never read here) and stores eight constants
// (320, 640, 768, 512, 50, 100, 90, 80) into the unit's globals. The header that defines those
// eight values is shared with App.cpp, CGameApp.cpp and about fifteen more units and is not known
// yet, so they are not reproduced here.
// The shared table is now recovered in AdBannerSizes.h.
#include "AdBannerSizes.h"
#include "ICInAppPurchase.h"
