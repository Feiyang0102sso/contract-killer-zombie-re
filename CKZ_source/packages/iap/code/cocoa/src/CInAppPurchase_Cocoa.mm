// CInAppPurchase_Cocoa.mm - ICInAppPurchase on StoreKit.
// Every store operation bumps m_pendingOperations; the matching completion callback drops it again.
#include "CInAppPurchase_Cocoa.h"

#import <StoreKit/StoreKit.h>

#include "CInAppPurchaseProduct_Cocoa.h"
#include "CInAppPurchaseTransaction_Cocoa.h"
#include "CProductsRequestDelegate_Cocoa.h"
#include "CStringUtils.h"
#include "CTransactionObserver_Cocoa.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::core::CVector;

ICInAppPurchase* ICInAppPurchase::CreateInstance()
{
    return new CInAppPurchase_Cocoa();
}

CInAppPurchase_Cocoa::CInAppPurchase_Cocoa()
    : m_transactionObserver(nil),
      m_productsRequestDelegate(nil),
      m_pendingOperations(0),
      m_delegate(NULL),
      m_restoredTransactions(),
      m_requestTimeout(NoRequestTimeout),
      m_resetRequestTimer(0),
      m_requestTimerActive(0),
      m_products()
{
}

CInAppPurchase_Cocoa::~CInAppPurchase_Cocoa()
{
    if (m_productsRequestDelegate != nil) {
        [m_productsRequestDelegate release];
        m_productsRequestDelegate = nil;
    }
    if (m_transactionObserver != nil) {
        [[SKPaymentQueue defaultQueue] removeTransactionObserver:m_transactionObserver];
        [m_transactionObserver release];
        m_transactionObserver = nil;
    }
}

void CInAppPurchase_Cocoa::Init(ICInAppPurchaseDelegate* delegate)
{
    m_delegate = delegate;
    m_transactionObserver = [[CTransactionObserver_Cocoa alloc] initWithIAP:this];
    [[SKPaymentQueue defaultQueue] addTransactionObserver:m_transactionObserver];
    m_productsRequestDelegate = [[CProductsRequestDelegate_Cocoa alloc] initWithIAP:this];
}

void CInAppPurchase_Cocoa::SyncTransactions()
{
    m_pendingOperations++;
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

void CInAppPurchase_Cocoa::RequestProductData(const CVector<CStrWChar>& productIds, int timeout)
{
    NSMutableSet* identifiers = [[NSMutableSet alloc] initWithCapacity:productIds.Size()];
    for (int index = 0; index < productIds.Size(); index++) {
        NSString* identifier = CStringUtils::CStrWCharToNSString(productIds[index]);
        [identifiers addObject:identifier];
        [identifier release];
    }

    NSSet* identifierSet = [[NSSet alloc] initWithSet:identifiers];
    m_productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:identifierSet];
    [m_productsRequest setDelegate:m_productsRequestDelegate];
    m_pendingOperations++;
    if (timeout > 0) {
        m_requestTimeout = timeout;
        m_requestTimerActive = 1;
        m_resetRequestTimer = 1;
    }
    [m_productsRequest start];
    [identifierSet release];
    [identifiers release];
}

void CInAppPurchase_Cocoa::BuyProduct(const CStrWChar& productId)
{
    NSString* identifier = CStringUtils::CStrWCharToNSString(productId);
    SKPayment* payment = [SKPayment paymentWithProductIdentifier:identifier];
    m_pendingOperations++;
    [[SKPaymentQueue defaultQueue] addPayment:payment];
    [identifier release];
}

unsigned char CInAppPurchase_Cocoa::IsTransactionActive() const
{
    return m_pendingOperations != 0;
}

unsigned char CInAppPurchase_Cocoa::IsAvailable() const
{
    return [SKPaymentQueue canMakePayments];
}

unsigned char CInAppPurchase_Cocoa::AllowsConcurrentOperations() const
{
    return 1;
}

void CInAppPurchase_Cocoa::HandleUpdate(int deltaTime)
{
    if (m_requestTimerActive) {
        if (m_resetRequestTimer) {
            m_requestElapsed = 0;
            m_resetRequestTimer = 0;
        }
        m_requestElapsed += deltaTime;
        if (m_requestElapsed > m_requestTimeout) {
            // timed out: give up on the request
            m_requestTimerActive = 0;
            [m_productsRequest cancel];
            RequestFailed(NULL, NULL);
            m_productsRequest = nil;
        }
    }
}

void CInAppPurchase_Cocoa::RequestFailed(SKProductsRequest* request, NSError* error)
{
    m_requestTimeout = NoRequestTimeout;
    m_requestElapsed = 0;
    m_requestTimerActive = 0;
    m_resetRequestTimer = 0;

    for (int index = 0; index < m_products.Size(); index++) {
        [m_products[index] release];
    }
    m_products.Clear();

    CVector<const ICInAppPurchaseProduct*> noProducts;
    ICInAppPurchaseDelegate::RequestProductDataState state = ICInAppPurchaseDelegate::REQUEST_PRODUCT_DATA_FAILED;
    // the timeout path (HandleUpdate) passes no request
    if (request == nil) {
        state = ICInAppPurchaseDelegate::REQUEST_PRODUCT_DATA_TIMEOUT;
    }
    m_delegate->RequestProductDataComplete(state, noProducts);
    m_pendingOperations--;
}

void CInAppPurchase_Cocoa::ValidProduct(const SKProductsRequest* request, SKProduct* product)
{
    [product retain];
    m_products.Add(product);
}

void CInAppPurchase_Cocoa::InvalidProduct(const SKProductsRequest* request, const NSString* productId)
{
}

void CInAppPurchase_Cocoa::UpdatedTransaction(SKPaymentTransaction* transaction)
{
    switch ([transaction transactionState]) {
    case SKPaymentTransactionStatePurchased: {
        CInAppPurchaseTransaction_Cocoa purchasedTransaction(transaction);
        m_delegate->BuyProductComplete(ICInAppPurchaseDelegate::BUY_PRODUCT_SUCCESS, &purchasedTransaction);
        m_pendingOperations--;
        break;
    }
    case SKPaymentTransactionStateFailed: {
        NSString* errorDescription = [[transaction error] localizedDescription];  // fetched but unused (as in the original)
        int errorCode = [[transaction error] code];
        CInAppPurchaseTransaction_Cocoa failedTransaction(transaction);
        ICInAppPurchaseDelegate::BuyProductState buyState = ICInAppPurchaseDelegate::BUY_PRODUCT_FAILED;
        if (errorCode == SKErrorPaymentCancelled) {
            buyState = ICInAppPurchaseDelegate::BUY_PRODUCT_CANCELLED;
        }
        m_delegate->BuyProductComplete(buyState, &failedTransaction);
        m_pendingOperations--;
        break;
    }
    case SKPaymentTransactionStateRestored:
        // reported all at once by RestoreCompleted
        [transaction retain];
        m_restoredTransactions.Add(transaction);
        break;
    default:
        break;
    }
}

void CInAppPurchase_Cocoa::RestoreFailed(const NSError* error)
{
    for (int index = 0; index < m_restoredTransactions.Size(); index++) {
        [m_restoredTransactions[index] release];
    }
    m_restoredTransactions.Clear();

    CVector<const ICInAppPurchaseTransaction*> noTransactions;
    m_delegate->SyncTransactionsComplete(ICInAppPurchaseDelegate::SYNC_TRANSACTIONS_FAILED, noTransactions);
    m_pendingOperations--;
}

void CInAppPurchase_Cocoa::RestoreCompleted()
{
    CVector<const ICInAppPurchaseTransaction*> transactions;
    for (int index = 0; index < m_restoredTransactions.Size(); index++) {
        const ICInAppPurchaseTransaction* transaction = new CInAppPurchaseTransaction_Cocoa(m_restoredTransactions[index]);
        transactions.Add(transaction);
        [m_restoredTransactions[index] release];
    }
    m_restoredTransactions.Clear();

    m_delegate->SyncTransactionsComplete(ICInAppPurchaseDelegate::SYNC_TRANSACTIONS_SUCCESS, transactions);

    for (int index = 0; index < transactions.Size(); index++) {
        delete transactions[index];
    }
    transactions.Clear();
    m_pendingOperations--;
}

void CInAppPurchase_Cocoa::RequestComplete(SKProductsRequest* request)
{
    CVector<const ICInAppPurchaseProduct*> products;
    m_requestTimeout = NoRequestTimeout;
    m_requestElapsed = 0;
    m_requestTimerActive = 0;
    m_resetRequestTimer = 0;

    for (int index = 0; index < m_products.Size(); index++) {
        const ICInAppPurchaseProduct* product = new CInAppPurchaseProduct_Cocoa(m_products[index]);
        products.Add(product);
        [m_products[index] release];
    }
    m_products.Clear();

    m_delegate->RequestProductDataComplete(ICInAppPurchaseDelegate::REQUEST_PRODUCT_DATA_SUCCESS, products);

    for (int index = 0; index < products.Size(); index++) {
        delete products[index];
    }
    products.Clear();
    m_pendingOperations--;
}
