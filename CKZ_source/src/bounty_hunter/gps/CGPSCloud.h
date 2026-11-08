// b082: true CGPSItem derivative with a third owning presenter at d0; full size124.
#ifndef CGPSCLOUD_H
#define CGPSCLOUD_H
#include "CGPSItem.h"
class CGPSCloud : public CGPSItem {
public:
    CGPSCloud(float x, float y, float velocityX, float velocityY);
    virtual ~CGPSCloud();
    virtual void SetPressed(bool) {} // Original weak 146bb4 is return-only.
    virtual void Update(long elapsed);
    virtual void Save(TiXmlElement* element);
    virtual void OnClick();
    virtual void Paint(com::glu::platform::graphics::ICGraphics2d& graphics, int x, int y);
    virtual void Load(TiXmlElement* element);
    void SetMapSize(int width, int height);
    SG_Presenter m_cloudPresenter; // d0
    float m_velocityX; // 10c
    float m_velocityY; // 110
    int m_mapWidth;    // 114
    int m_mapHeight;   // 118
    int m_offsetX;     // 11c
    int m_offsetY;     // 120
};
#endif
