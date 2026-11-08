// b082: original Shop and single-slot listener; no listener virtual destructor.
#ifndef CGPSSHOP_H
#define CGPSSHOP_H
#include "CGPSItem.h"
class CGPSShop;
class CGPSShopListener {
public:
    virtual void OnShopItemSelected(CGPSShop* shop) = 0;
};
class CGPSShop : public CGPSItem {
public:
    CGPSShop();
    virtual ~CGPSShop();
    virtual void SetBlinking(bool blinking);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* element);
    virtual void OnClick();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void Load(TiXmlElement* element);
    void SetListener(CGPSShopListener* listener);
    CGPSShopListener* m_listener; // d0; full object d4
};
#endif
