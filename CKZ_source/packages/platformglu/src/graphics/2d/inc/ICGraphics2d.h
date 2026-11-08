// ICGraphics2d.h - 2d drawing interface of a graphics abstraction (created by b037x, owner of
// ICGraphics2d::GetInstance in graphics/src/CGraphicsAbstractionManager.cpp).
// Slot order from the vtable of CGraphics2d_OGLES.
#ifndef ICGRAPHICS2D_H
#define ICGRAPHICS2D_H

#include "CClass.h"
#include "ICGraphicsAbstractionManager.h"

namespace com { namespace glu { namespace platform {

namespace math {
class CMatrix2d;
class CVector2d;
class CLineSegment2d;
}

namespace components {
template <class T> class CRectangle;
struct Color_RGBA;
}

namespace graphics {

class ICRenderSurface;
class ICRenderText2d;

class ICGraphics2d : public core::CClass {
public:
    /** Runtime class id of the 2d renderers. */
    enum { ClassId = 0x66e79740 };

    /** Mirroring of a drawn surface. */
    enum Flip {
        Flip_None = 0,
        Flip_Horizontal = 1,
        Flip_Vertical = 2,
        Flip_Both = 3
    };

    /** Primitive kinds of Draw(Primitive, ...). Values from CGraphics2d_OGLES::Draw (fx1);
        0 and 2 are not handled there, their names are unknown. */
    enum Primitive {
        Primitive_Lines = 1,
        Primitive_Rectangles = 3
    };

    /** The 2d renderer of the application's abstraction manager. */
    static ICGraphics2d* GetInstance();

    ICGraphics2d() : core::CClass(ClassId) {}

    virtual ~ICGraphics2d() {}

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const = 0;
    virtual unsigned char Initialize() = 0;
    virtual void Destroy() = 0;
    virtual void LoadIdentity() = 0;
    virtual void LoadTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation) = 0;
    virtual void MultiplyTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation) = 0;
    virtual void Translate(float x, float y) = 0;
    virtual void Rotate(float angle) = 0;
    virtual void Scale(float x, float y) = 0;
    virtual void PushTransform() = 0;
    virtual void PopTransform() = 0;
    virtual void GetTransform(const math::CMatrix2d** matrix, const math::CVector2d** translation) = 0;
    virtual void SetVertexPositionCoords(int count, unsigned int type, int stride, const void* coords) = 0;
    virtual void Draw(ICRenderSurface& surface, const components::CRectangle<short>* source,
                      const components::Color_RGBA* color, Flip flip) = 0;
    virtual void Draw(ICRenderSurface& surface, float x, float y, const components::CRectangle<short>* source,
                      const components::Color_RGBA* color, Flip flip) = 0;
    virtual void Draw(ICRenderSurface& surface, unsigned int width, unsigned int height,
                      const components::CRectangle<short>* source, const components::Color_RGBA* color, Flip flip) = 0;
    virtual void Draw(ICRenderSurface& surface, float x, float y, unsigned int width, unsigned int height,
                      const components::CRectangle<short>* source, const components::Color_RGBA* color, Flip flip) = 0;
    virtual void Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color) = 0;
    virtual void Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color,
                      float x, float y) = 0;
    virtual void Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color) = 0;
    virtual void Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color,
                      float x, float y) = 0;
    virtual void Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color) = 0;
    virtual void Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color,
                      float x, float y) = 0;
    virtual void Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color) = 0;
    virtual void Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color, float x, float y) = 0;
    virtual void Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color) = 0;
    virtual void Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color,
                      float x, float y) = 0;
    virtual void Draw(ICRenderText2d& text, const components::Color_RGBA* color) = 0;
};

} } } }

#endif
