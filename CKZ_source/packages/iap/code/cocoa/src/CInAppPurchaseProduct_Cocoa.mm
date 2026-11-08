// CInAppPurchaseProduct_Cocoa.mm - ICInAppPurchaseProduct backed by an SKProduct.
#include "CInAppPurchaseProduct_Cocoa.h"

#import <StoreKit/StoreKit.h>

#include "CStringUtils.h"

using com::glu::platform::components::CStrWChar;

CInAppPurchaseProduct_Cocoa::CInAppPurchaseProduct_Cocoa(SKProduct* product)
    : m_product(product)
{
    [product retain];
}

CInAppPurchaseProduct_Cocoa::~CInAppPurchaseProduct_Cocoa()
{
    [m_product release];
}

CStrWChar CInAppPurchaseProduct_Cocoa::GetDescription() const
{
    return CStringUtils::NSStringToCStrWChar([m_product localizedDescription]);
}

CStrWChar CInAppPurchaseProduct_Cocoa::GetTitle() const
{
    return CStringUtils::NSStringToCStrWChar([m_product localizedTitle]);
}

CStrWChar CInAppPurchaseProduct_Cocoa::GetProductIdentifier() const
{
    return CStringUtils::NSStringToCStrWChar([m_product productIdentifier]);
}

float CInAppPurchaseProduct_Cocoa::GetPrice() const
{
    return [[m_product price] floatValue];
}

CStrWChar CInAppPurchaseProduct_Cocoa::GetCurrencySymbol() const
{
    return CStringUtils::NSStringToCStrWChar([[m_product priceLocale] objectForKey:NSLocaleCurrencySymbol]);
}

CStrWChar CInAppPurchaseProduct_Cocoa::GetLocaleIdentifier() const
{
    return CStringUtils::NSStringToCStrWChar([[m_product priceLocale] localeIdentifier]);
}
