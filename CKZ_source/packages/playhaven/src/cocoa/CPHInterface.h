// CPHInterface.h - C++ entry points of the PlayHaven integration (created by b033x).
#ifndef CPHINTERFACE_H
#define CPHINTERFACE_H

#ifdef __OBJC__
@class CPHManager;
#else
typedef struct objc_object CPHManager;
#endif

class CPHInterface {
public:
    /** Create the manager and send the publisher open request. */
    static void init(const char* token, const char* secret);
    static void destroy();
    static void startPublisherContentRequest(const char* placement, bool showsOverlayImmediately, bool showErrorMessage);
    static bool isPublisherContentRequestActive(const char* placement);
    static void showNotificationView(const char* placement, bool testMode, unsigned int centerX, unsigned int centerY);
    static void hideNotificationView(const char* placement);

    static CPHManager* m_pPHManager;
};

#endif
