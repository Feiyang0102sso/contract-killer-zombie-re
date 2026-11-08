// CTransactionObserver_Cocoa.h - SKPaymentQueue observer forwarding to CInAppPurchase_Cocoa (created by b033x).
#ifndef CTRANSACTIONOBSERVER_COCOA_H
#define CTRANSACTIONOBSERVER_COCOA_H

#import <StoreKit/StoreKit.h>

class CInAppPurchase_Cocoa;

@interface CTransactionObserver_Cocoa : NSObject <SKPaymentTransactionObserver> {
    CInAppPurchase_Cocoa* m_iap;    // not retained (owner)
}

- (id)initWithIAP:(CInAppPurchase_Cocoa*)iap;

@end

#endif
