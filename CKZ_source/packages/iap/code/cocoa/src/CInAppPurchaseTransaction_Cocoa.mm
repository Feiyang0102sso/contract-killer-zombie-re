// CInAppPurchaseTransaction_Cocoa.mm - ICInAppPurchaseTransaction backed by an SKPaymentTransaction.
#include "CInAppPurchaseTransaction_Cocoa.h"

#import <StoreKit/StoreKit.h>

#include "CStringUtils.h"

using com::glu::platform::components::CStrWChar;

CInAppPurchaseTransaction_Cocoa::CInAppPurchaseTransaction_Cocoa(const SKPaymentTransaction* transaction)
    : m_transaction(transaction)
{
    [(SKPaymentTransaction*)transaction retain];
}

CInAppPurchaseTransaction_Cocoa::~CInAppPurchaseTransaction_Cocoa()
{
    [(SKPaymentTransaction*)m_transaction release];
}

int CInAppPurchaseTransaction_Cocoa::GetTransactionState() const
{
    return [m_transaction transactionState];
}

CStrWChar CInAppPurchaseTransaction_Cocoa::GetProductIdentifier() const
{
    return CStringUtils::NSStringToCStrWChar([[m_transaction payment] productIdentifier]);
}

CStrWChar CInAppPurchaseTransaction_Cocoa::GetTransactionIdentifier() const
{
    CStrWChar identifier;
    if ([m_transaction transactionState] == SKPaymentTransactionStatePurchased ||
        [m_transaction transactionState] == SKPaymentTransactionStateRestored) {
        identifier = CStringUtils::NSStringToCStrWChar([m_transaction transactionIdentifier]);
    }
    return identifier;
}

CStrWChar CInAppPurchaseTransaction_Cocoa::GetErrorDescription() const
{
    CStrWChar description;
    if ([m_transaction transactionState] == SKPaymentTransactionStateFailed) {
        description = CStringUtils::NSStringToCStrWChar([[m_transaction error] localizedDescription]);
    }
    return description;
}

CStrWChar CInAppPurchaseTransaction_Cocoa::GetReceiptDescription() const
{
    CStrWChar description;
    if ([m_transaction transactionState] == SKPaymentTransactionStatePurchased) {
        description = CStringUtils::NSStringToCStrWChar([[m_transaction transactionReceipt] description]);
    }
    return description;
}

CStrWChar CInAppPurchaseTransaction_Cocoa::GetDate() const
{
    CStrWChar date;
    if ([m_transaction transactionState] == SKPaymentTransactionStatePurchased ||
        [m_transaction transactionState] == SKPaymentTransactionStateRestored) {
        date = CStringUtils::NSStringToCStrWChar([[m_transaction transactionDate] description]);
    }
    return date;
}
