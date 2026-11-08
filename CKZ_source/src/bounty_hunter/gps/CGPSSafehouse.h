// b082: source owner is CGPSSafeHouse.cpp, original class spelling is CGPSSafehouse.
#ifndef CGPSSAFEHOUSE_H
#define CGPSSAFEHOUSE_H
#include "CGPSItem.h"
class CGPSSafehouse;
class CGPSSafehouseListener {
public:
    virtual void OnSafehouseItemSelected(CGPSSafehouse* safehouse) = 0;
};
class CGPSSafehouse : public CGPSItem {
public:
    CGPSSafehouse();
    virtual ~CGPSSafehouse();
    virtual void SetBlinking(bool blinking);
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* element);
    virtual void OnClick();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void Load(TiXmlElement* element);
    void SetListener(CGPSSafehouseListener* listener);
    CGPSSafehouseListener* m_listener; // d0; full object d4
};
#endif
