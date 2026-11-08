// ICCore.h - platform core services: tick rate, exit, URL launching, language/locale, device and
// interface orientation, auto-rotation, platform identity, touch, accelerometer, gyroscope.
// One instance per application (CApplet::m_pCore), created by ICCore::CreateInstance()
// (CCore_Cocoa on iOS). Written by b002, the owner (CCore_Cocoa.cpp).
#ifndef ICCORE_H
#define ICCORE_H

#include "CClass.h"
#include "CVector2dx.h"

namespace com { namespace glu { namespace platform {

namespace components {
class CStrChar;
class CStrWChar;
}

namespace core {
/** Key codes of the input system (only the values used here are named). */
enum eKeyCode {
    KeyCode_SoftkeyLeft = 13
};
}

namespace framework {

class ICCore : public core::CClass {
public:
    /** ExitApp() reasons. */
    enum eExitStatus {
        ExitStatus_Normal = 0
    };

    /**
     * Device orientation in quarter turns: 1 is the default (portrait) orientation, 2 and 4 the
     * two landscape orientations, 3 upside down. 5 asks for the platform default.
     */
    enum eDeviceOrientation {
        DeviceOrientation_Unknown = 0,
        DeviceOrientation_Portrait = 1,
        DeviceOrientation_LandscapeRight = 2,
        DeviceOrientation_PortraitUpsideDown = 3,
        DeviceOrientation_LandscapeLeft = 4,
        DeviceOrientation_Default = 5
    };

    /** Orientations the user interface may auto-rotate to (index into the enable tables). */
    enum eAutoRotationOrientation {
        AutoRotation_None = 0,
        AutoRotation_Portrait = 1,
        AutoRotation_LandscapeRight = 2,
        AutoRotation_PortraitUpsideDown = 3,
        AutoRotation_LandscapeLeft = 4,
        AutoRotation_All = 5,
        AutoRotation_Count = 6
    };

    /** Where a soft key label is drawn. */
    enum eSoftkeyPosition {
        SoftkeyPosition_BottomLeft = 3,
        SoftkeyPosition_BottomRight = 4
    };

    /** Platform factory (CCore_Cocoa on iOS). */
    static ICCore* CreateInstance();

    /** Index of `orientation` into the per-orientation tables (0 when it is not a real one). */
    static int DeviceOrientationIndex(eDeviceOrientation orientation)
    {
        int index = DeviceOrientation_Unknown;
        switch (orientation) {
        case DeviceOrientation_Portrait:
            index = DeviceOrientation_Portrait;
            break;
        case DeviceOrientation_LandscapeRight:
            index = DeviceOrientation_LandscapeRight;
            break;
        case DeviceOrientation_PortraitUpsideDown:
            index = DeviceOrientation_PortraitUpsideDown;
            break;
        case DeviceOrientation_LandscapeLeft:
            index = DeviceOrientation_LandscapeLeft;
            break;
        default:
            break;
        }
        return index;
    }

    virtual ~ICCore() {}

    /** Period of the application timer in milliseconds. */
    virtual void SetTickRate(unsigned int milliseconds) = 0;
    virtual void ExitApp(eExitStatus status) = 0;
    virtual unsigned char DeviceCanMultitask() const = 0;
    /** Open `url` in the system browser. */
    virtual unsigned char LaunchURL(const wchar_t* url, unsigned char exitApplication) = 0;
    virtual unsigned char CanDetectLanguage() const = 0;
    /** Preferred language code ("en", "fr", ...). */
    virtual unsigned char DetectLanguage(components::CStrChar& language) const = 0;
    virtual unsigned char CanDetectLocale() const = 0;
    /** Locale identifier ("en_US", ...). */
    virtual unsigned char DetectLocale(components::CStrChar& locale) const = 0;
    virtual unsigned char ApplicationCanSetOrientation() const = 0;
    virtual eDeviceOrientation GetDeviceOrientation() const = 0;
    virtual unsigned char CanSetDeviceOrientation(eDeviceOrientation orientation) const = 0;
    virtual unsigned char SetDeviceOrientation(eDeviceOrientation orientation) = 0;
    virtual unsigned char DeviceOrientationCanSetRotateLeft() const = 0;
    virtual unsigned char DeviceOrientationCanSetRotateRight() const = 0;
    virtual unsigned char DeviceOrientationCanSetFlip() const = 0;
    virtual unsigned char DeviceOrientationCanSetDefault() const = 0;
    virtual unsigned char DeviceOrientationSetRotateLeft() = 0;
    virtual unsigned char DeviceOrientationSetRotateRight() = 0;
    virtual unsigned char DeviceOrientationSetFlip() = 0;
    virtual unsigned char DeviceOrientationSetDefault() = 0;
    virtual unsigned char AutoRotationIsActive() const = 0;
    virtual unsigned char ApplicationCanSelectivelyEnableAllAutoRotationOrientations() const = 0;
    virtual unsigned char ApplicationCanSelectivelyEnableAutoRotationOrientation(eAutoRotationOrientation orientation) const = 0;
    virtual eAutoRotationOrientation GetDisplayOrientationInTermsOfAutoRotation() const = 0;
    virtual unsigned char AutoRotationOrientationIsEnabled(eAutoRotationOrientation orientation) const = 0;
    virtual unsigned char AutoRotationOrientationEnable(eAutoRotationOrientation orientation) = 0;
    virtual unsigned char AutoRotationOrientationDisable(eAutoRotationOrientation orientation) = 0;
    virtual eSoftkeyPosition GetSoftkeyPosition(core::eKeyCode key) const = 0;
    /** Persistent per-device identifier (en0 MAC address or a stored UUID). */
    virtual components::CStrWChar GetDeviceIdentifier() const = 0;
    /** Hardware model ("iPhone3,1"). */
    virtual components::CStrWChar GetPlatformIdentifier() const = 0;
    /** Operating system version ("5.0.1"). */
    virtual components::CStrWChar GetPlatformVersion() const = 0;
    virtual components::CStrWChar GetPlatformName() const = 0;
    virtual const math::CVector2dx& GetDisplayToWindowScale() const = 0;
    virtual const math::CVector2dx& GetWindowToDisplayScale() const = 0;
    /** Accelerometer samples per second (0 stops the updates). */
    virtual void SetAccelerometerFrequency(unsigned int frequency) = 0;
    virtual unsigned char CanMultipleTouch() const = 0;
    virtual unsigned char IsMultipleTouchEnabled() const = 0;
    virtual void SetMultipleTouchEnabled(unsigned char enabled) = 0;
    /** Keep the screen lit (disables the idle timer when false). */
    virtual unsigned char SetBacklightEnabled(unsigned char enabled) = 0;
    virtual unsigned char IsGyroscopeAvailable() const = 0;
    virtual void SetGyroscopeFrequency(unsigned int frequency) = 0;
    /** Free physical memory in bytes. */
    virtual unsigned int GetDeviceAvailableMemory() = 0;
    virtual void SystemMessageBox(const components::CStrWChar& title, const components::CStrWChar& message) const = 0;
    virtual components::CStrWChar GetDeviceModel() const = 0;
    virtual unsigned char IsNetworkAvailable() const = 0;
    virtual unsigned char CanDetectTimezone() const = 0;
    virtual components::CStrWChar GetTimezone() const = 0;

    // ---- state shared with the platform layer and the renderers (set up by CCore) ----
    int m_maxTouches;                                   // 0x08 6
    int m_heapBudget;                                   // 0x0c 0x1e951a0 (meaning not confirmed)
    int m_watchdogMillis;                               // 0x10 30000 (meaning not confirmed)
    unsigned char m_hasAccelerometer;                   // 0x14
    unsigned char m_hasTouchScreen;                     // 0x15
    unsigned char m_hasTelephony;                       // 0x16 running on an iPhone
    unsigned char m_canVibrate;                         // 0x17 running on an iPhone
    unsigned char m_keyboardType;                       // 0x18 3
    unsigned char m_canSetDeviceOrientation[5];         // 0x19 indexed by DeviceOrientationIndex()
    unsigned char m_applicationCanSetOrientation;       // 0x1e
    unsigned char m_orientationFlags1f[2];              // 0x1f cleared by the OGLES device (meaning unknown)
    unsigned char m_reserved21[3];                      // 0x21
    unsigned char m_deviceOrientationSet;               // 0x24 m_deviceOrientation is valid
    eDeviceOrientation m_deviceOrientation;             // 0x28
    unsigned char m_autoRotationIsActive;               // 0x2c
    unsigned char m_autoRotationAllowed;                // 0x2d
    unsigned char m_autoRotationOrientationEnabled[AutoRotation_Count];   // 0x2e
    math::CVector2dx m_windowToDisplayScale;            // 0x34
    math::CVector2dx m_displayToWindowScale;            // 0x3c
};

} } } }

#endif
