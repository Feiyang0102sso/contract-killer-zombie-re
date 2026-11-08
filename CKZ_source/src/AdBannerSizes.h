// PROVISIONAL (created by b047x): the original header of this table is not known
// AdBannerSizes.h - banner sizes of the ad views (iPhone, iPhone retina, iPad, iPad half size).
// Many game units (App.cpp, CGameApp.cpp, CGServeHandler.cpp, CNotificationHandler.cpp, ...) carry a
// private copy of a 32 byte table that their static initializer fills with 320, 640, 768, 512 /
// 50, 100, 90, 80. Names are guesses.
// b111: the original keeps two separate 16 byte statics (widths, heights): the compiler's GlobalMerge
// combines them into a 32 byte __MergedGlobals, which a single 32 byte object never produces.
#ifndef ADBANNERSIZES_H
#define ADBANNERSIZES_H

/** Ad banner widths: phone, phone retina, pad, pad half. */
struct AdBannerWidths {
    AdBannerWidths()
    {
        m_value[0] = 320;
        m_value[1] = 640;
        m_value[2] = 768;
        m_value[3] = 512;
    }

    int m_value[4];
};

/** Ad banner heights, same order as the widths. */
struct AdBannerHeights {
    AdBannerHeights()
    {
        m_value[0] = 50;
        m_value[1] = 100;
        m_value[2] = 90;
        m_value[3] = 80;
    }

    int m_value[4];
};

/** This unit's copies (filled by the static initializer). */
static AdBannerWidths s_adBannerWidths;
static AdBannerHeights s_adBannerHeights;

#endif
