// CProductsRequestDelegate_Cocoa.h - SKProductsRequest delegate forwarding to CInAppPurchase_Cocoa (created by b033x).
#ifndef CPRODUCTSREQUESTDELEGATE_COCOA_H
#define CPRODUCTSREQUESTDELEGATE_COCOA_H

#import <StoreKit/StoreKit.h>

class CInAppPurchase_Cocoa;

@interface CProductsRequestDelegate_Cocoa : NSObject <SKProductsRequestDelegate> {
    CInAppPurchase_Cocoa* m_iap;    // not retained (owner)
}

- (id)initWithIAP:(CInAppPurchase_Cocoa*)iap;

@end

#endif
