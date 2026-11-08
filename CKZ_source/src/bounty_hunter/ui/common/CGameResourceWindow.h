// b074i: original 14888c real Button inheritance; only external declarations/layout, the full body belongs to the same-named cpp.
#ifndef CGAMERESOURCEWINDOW_H
#define CGAMERESOURCEWINDOW_H
#include "CBH_Button.h"
class TextWindow;
class SGImageWindow;

class CGameResourceWindow : public CBH_Button {
public:
    enum EType { TYPE_0 = 0, TYPE_1 = 1, TYPE_2 = 2, TYPE_7 = 7 };
    CGameResourceWindow(EType type, bool flag);
    virtual ~CGameResourceWindow() {} // Original 148ca0 is a real empty destructor; child windows are owned by the tree.
    virtual void Update();
    void UpdateDisplay();
    void SetValues(int value, int maximum);
    // Historical unsigned char m_unresolvedE4[0xf8 - 0xe4], now promoted to real members based on the full owner.
    EType m_type;                  // e4
    TextWindow* m_valueText;       // e8
    SGImageWindow* m_icon;         // ec
    int m_value;                  // f0
    int m_maximum;                // f4
    int m_displayFormat; // f8, initialized by base at original 148a54 and read by base at 148aa0; total fc.
};

class CPlayerGameResourceWindow : public CGameResourceWindow {
public:
    CPlayerGameResourceWindow(EType type, bool flag);
    virtual ~CPlayerGameResourceWindow() {} // Original 148cc4 only enters the base class destructor.
    virtual void Update();
    // Historical int m_resourceType; 0xf8, the player stat selector value used by 148c08, total fc.
    // The derived constructor rewrites base's m_displayFormat; the original adds no new field; base/derived are both fc.
};
#endif
