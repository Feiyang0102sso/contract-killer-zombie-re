// CCore.h - platform independent part of ICCore: default state and the orientation helpers that
// are expressed through the platform's primitive virtuals (header-only).
// Created by b002 (no owning unit: the inline bodies are emitted by cocoa/src/CCore_Cocoa.cpp).
#ifndef CCORE_H
#define CCORE_H

#include "ICCore.h"

namespace com { namespace glu { namespace platform { namespace framework {

class CCore : public ICCore {
public:
    /** Runtime class id of the platform core. */
    static const unsigned int ClassId = 0x02d75845;

    CCore()
    {
        m_classId = ClassId;
        m_canSetDeviceOrientation[0] = 0;
        m_canSetDeviceOrientation[1] = 1;
        m_canSetDeviceOrientation[2] = 0;
        m_canSetDeviceOrientation[3] = 0;
        m_canSetDeviceOrientation[4] = 0;
        m_applicationCanSetOrientation = 0;
        m_deviceOrientationSet = 0;
        m_deviceOrientation = DeviceOrientation_Portrait;
        for (int index = 0; index < AutoRotation_Count; index++) {
            m_autoRotationOrientationEnabled[index] = 0;
        }
        m_windowToDisplayScale.x = 0x10000;
        m_windowToDisplayScale.y = 0x10000;
        m_displayToWindowScale.x = 0x10000;
        m_displayToWindowScale.y = 0x10000;
    }

    virtual unsigned char DeviceOrientationCanSetRotateLeft() const
    {
        return CanSetDeviceOrientation(RotatedLeft(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationCanSetRotateRight() const
    {
        return CanSetDeviceOrientation(RotatedRight(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationCanSetFlip() const
    {
        return CanSetDeviceOrientation(Flipped(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationCanSetDefault() const
    {
        return CanSetDeviceOrientation(DeviceOrientation_Default);
    }

    virtual unsigned char DeviceOrientationSetRotateLeft()
    {
        return SetDeviceOrientation(RotatedLeft(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationSetRotateRight()
    {
        return SetDeviceOrientation(RotatedRight(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationSetFlip()
    {
        return SetDeviceOrientation(Flipped(GetDeviceOrientation()));
    }

    virtual unsigned char DeviceOrientationSetDefault()
    {
        return SetDeviceOrientation(DeviceOrientation_Default);
    }

    virtual unsigned char ApplicationCanSelectivelyEnableAllAutoRotationOrientations() const
    {
        return m_autoRotationOrientationEnabled[AutoRotation_All];
    }

    virtual unsigned char ApplicationCanSelectivelyEnableAutoRotationOrientation(eAutoRotationOrientation orientation) const
    {
        return ApplicationCanSelectivelyEnableAllAutoRotationOrientations();
    }

    virtual unsigned char AutoRotationOrientationIsEnabled(eAutoRotationOrientation orientation) const
    {
        return m_autoRotationOrientationEnabled[orientation];
    }

    /** Enable one auto-rotation orientation; 0 when the application may not choose. */
    virtual unsigned char AutoRotationOrientationEnable(eAutoRotationOrientation orientation)
    {
        if (!ApplicationCanSelectivelyEnableAutoRotationOrientation(orientation)) {
            return 0;
        }
        if (m_autoRotationOrientationEnabled[orientation]) {
            return 1;
        }
        m_autoRotationOrientationEnabled[orientation] = 1;
        UpdateAutoRotationOrientation();
        return 1;
    }

    /** Disable one auto-rotation orientation; 0 when the application may not choose. */
    virtual unsigned char AutoRotationOrientationDisable(eAutoRotationOrientation orientation)
    {
        if (!ApplicationCanSelectivelyEnableAutoRotationOrientation(orientation)) {
            return 0;
        }
        if (!m_autoRotationOrientationEnabled[orientation]) {
            return 1;
        }
        m_autoRotationOrientationEnabled[orientation] = 0;
        UpdateAutoRotationOrientation();
        return 1;
    }

    virtual const math::CVector2dx& GetDisplayToWindowScale() const
    {
        return m_displayToWindowScale;
    }

    virtual const math::CVector2dx& GetWindowToDisplayScale() const
    {
        return m_windowToDisplayScale;
    }

    /** Apply a change of the enabled auto-rotation orientations. */
    virtual void UpdateAutoRotationOrientation() = 0;

private:
    /** Orientation one quarter turn to the left of `orientation`. */
    static eDeviceOrientation RotatedLeft(eDeviceOrientation orientation)
    {
        eDeviceOrientation rotated = DeviceOrientation_Unknown;
        switch (orientation) {
        case DeviceOrientation_Portrait:
            rotated = DeviceOrientation_LandscapeRight;
            break;
        case DeviceOrientation_LandscapeRight:
            rotated = DeviceOrientation_PortraitUpsideDown;
            break;
        case DeviceOrientation_PortraitUpsideDown:
            rotated = DeviceOrientation_LandscapeLeft;
            break;
        case DeviceOrientation_LandscapeLeft:
            rotated = DeviceOrientation_Portrait;
            break;
        default:
            break;
        }
        return rotated;
    }

    /** Orientation one quarter turn to the right of `orientation`. */
    static eDeviceOrientation RotatedRight(eDeviceOrientation orientation)
    {
        eDeviceOrientation rotated = DeviceOrientation_Unknown;
        switch (orientation) {
        case DeviceOrientation_Portrait:
            rotated = DeviceOrientation_LandscapeLeft;
            break;
        case DeviceOrientation_LandscapeRight:
            rotated = DeviceOrientation_Portrait;
            break;
        case DeviceOrientation_PortraitUpsideDown:
            rotated = DeviceOrientation_LandscapeRight;
            break;
        case DeviceOrientation_LandscapeLeft:
            rotated = DeviceOrientation_PortraitUpsideDown;
            break;
        default:
            break;
        }
        return rotated;
    }

    /** Orientation half a turn from `orientation`. */
    static eDeviceOrientation Flipped(eDeviceOrientation orientation)
    {
        eDeviceOrientation flipped = DeviceOrientation_Unknown;
        switch (orientation) {
        case DeviceOrientation_Portrait:
            flipped = DeviceOrientation_PortraitUpsideDown;
            break;
        case DeviceOrientation_LandscapeRight:
            flipped = DeviceOrientation_LandscapeLeft;
            break;
        case DeviceOrientation_PortraitUpsideDown:
            flipped = DeviceOrientation_Portrait;
            break;
        case DeviceOrientation_LandscapeLeft:
            flipped = DeviceOrientation_LandscapeRight;
            break;
        default:
            break;
        }
        return flipped;
    }
};

} } } }

#endif
