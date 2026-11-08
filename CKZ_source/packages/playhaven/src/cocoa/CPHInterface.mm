// CPHInterface.mm - C++ entry points of the PlayHaven integration; everything forwards to CPHManager.
#include "CPHInterface.h"

#import "CPHManager.h"

CPHManager* CPHInterface::m_pPHManager;

void CPHInterface::init(const char* token, const char* secret)
{
    m_pPHManager = [[CPHManager alloc] init:token :secret];
    if (m_pPHManager != nil) {
        [m_pPHManager startPublisherOpenRequest];
    }
}

void CPHInterface::destroy()
{
    if (m_pPHManager != nil) {
        // the original calls dealloc directly instead of release
        [m_pPHManager dealloc];
        m_pPHManager = nil;
    }
}

void CPHInterface::startPublisherContentRequest(const char* placement, bool showsOverlayImmediately, bool showErrorMessage)
{
    if (m_pPHManager != nil) {
        [m_pPHManager startPublisherContentRequest:placement :showsOverlayImmediately :showErrorMessage];
    }
}

bool CPHInterface::isPublisherContentRequestActive(const char* placement)
{
    bool active = false;
    if (m_pPHManager != nil) {
        active = [m_pPHManager isPublisherContentRequestActive:placement];
    }
    return active;
}

void CPHInterface::showNotificationView(const char* placement, bool testMode, unsigned int centerX, unsigned int centerY)
{
    if (m_pPHManager != nil) {
        [m_pPHManager showNotificationView:placement :testMode :centerX :centerY];
    }
}

void CPHInterface::hideNotificationView(const char* placement)
{
    if (m_pPHManager != nil) {
        [m_pPHManager hideNotificationView:placement];
    }
}
