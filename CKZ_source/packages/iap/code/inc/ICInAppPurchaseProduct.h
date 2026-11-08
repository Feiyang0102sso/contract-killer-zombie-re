// ICInAppPurchaseProduct.h - one product returned by an in-app purchase store query (created by b033x).
#ifndef ICINAPPPURCHASEPRODUCT_H
#define ICINAPPPURCHASEPRODUCT_H

#include "CStrWChar.h"

/** Read-only description of a store product (platform implementation: CInAppPurchaseProduct_Cocoa). */
class ICInAppPurchaseProduct {
public:
    virtual ~ICInAppPurchaseProduct() {}

    virtual com::glu::platform::components::CStrWChar GetDescription() const = 0;
    virtual com::glu::platform::components::CStrWChar GetTitle() const = 0;
    virtual com::glu::platform::components::CStrWChar GetProductIdentifier() const = 0;
    virtual float GetPrice() const = 0;
    virtual com::glu::platform::components::CStrWChar GetCurrencySymbol() const = 0;
    virtual com::glu::platform::components::CStrWChar GetLocaleIdentifier() const = 0;
};

#endif
