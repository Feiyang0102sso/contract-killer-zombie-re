// CTransactionObserver_Cocoa.mm - forwards SKPaymentQueue notifications to CInAppPurchase_Cocoa.
#include "CTransactionObserver_Cocoa.h"

#include "CInAppPurchase_Cocoa.h"

@implementation CTransactionObserver_Cocoa

- (id)init
{
    return [self initWithIAP:NULL];
}

- (id)initWithIAP:(CInAppPurchase_Cocoa*)iap
{
    self = [super init];
    if (self) {
        m_iap = iap;
    }
    return self;
}

- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions
{
    for (SKPaymentTransaction* transaction in transactions) {
        m_iap->UpdatedTransaction(transaction);
        // everything but an in-progress purchase is done once reported
        if ([transaction transactionState] != SKPaymentTransactionStatePurchasing) {
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
        }
    }
}

- (void)paymentQueue:(SKPaymentQueue*)queue removedTransactions:(NSArray*)transactions
{
}

- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error
{
    m_iap->RestoreFailed(error);
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue
{
    m_iap->RestoreCompleted();
}

@end
