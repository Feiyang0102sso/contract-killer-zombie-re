// CGraphics2d_OGLES.h - OpenGL ES implementation of the 2d renderer (b035, owner of
// graphics/2d/src/CGraphics2d_OGLES.cpp; first drafted by b002 as PROVISIONAL).
// The renderer draws through the device (CGraphics_OGLES): immediately with its own small vertex
// buffers, or as batch instructions of the display program the device is recording.
// Transforms are kept on a stack of 16.16 fixed point 2x3 matrices.
#ifndef CGRAPHICS2D_OGLES_H
#define CGRAPHICS2D_OGLES_H

#include <stddef.h>
#include "ICGraphics2d.h"
#include "CStack.h"
#include "CMatrix2d.h"
#include "CMatrix2dx.h"
#include "CMatrix4d.h"
#include "CVector2d.h"
#include "CVector2dx.h"
#include "CMathFixed.h"
#include "ARM_math.h"
#include "CDisplayProgram.h"

namespace com { namespace glu { namespace platform {

namespace systems {
class CEvent;
class CEventListener;
}

namespace graphics {

class CGraphics_OGLES;
class CRasterizerState_v1;
class ICShaderProgram;
class ICVertexBuffer;
class ICIndexBuffer;

/** Event callback of the renderer (surface / target changes, intrinsic programs loaded). */
unsigned char Graphics2d_OGLES_HandleEvent(systems::CEvent& event, void* listener);

/** Orthographic projection: maps [left, right] x [bottom, top] x [near, far] to the unit cube. */
void CGraphics2d_OGLES_Ortho(math::CMatrix4d& matrix, float left, float right, float bottom, float top,
                             float nearValue, float farValue);

class CGraphics2d_OGLES : public ICGraphics2d {
public:
    /** 2d transform in 16.16 fixed point: x' = m[0][0] x + m[0][1] y + tx, y' = m[1][0] x + m[1][1] y + ty. */
    class Matrix {
    public:
        /** Identity. */
        Matrix()
        {
            m[0][0] = 0x10000;
            m[1][0] = 0;
            m[0][1] = 0;
            m[1][1] = 0x10000;
            m_tx = 0;
            m_ty = 0;
        }

        /** [a b; c d] without translation. */
        Matrix(int a, int b, int c, int d)
        {
            m[0][0] = a;
            m[0][1] = b;
            m[1][0] = c;
            m[1][1] = d;
            m_tx = 0;
            m_ty = 0;
        }

        /** Reset to the identity. */
        void identity()
        {
            m[0][0] = 0x10000;
            m[0][1] = 0;
            m[1][0] = 0;
            m[1][1] = 0x10000;
            m_tx = 0;
            m_ty = 0;
        }

        /** Append the transform [a b; c d] + (tx, ty): the translation is applied first. */
        void multiply(int a, int b, int c, int d, int tx, int ty)
        {
            Matrix result;
            Matrix other;
            other.m[0][0] = a;
            other.m[0][1] = b;
            other.m[1][0] = c;
            other.m[1][1] = d;
            int translateX = arm::smult16(tx, m[0][0]) + arm::smult16(ty, m[0][1]);
            int translateY = arm::smult16(tx, m[1][0]) + arm::smult16(ty, m[1][1]);
            m_tx += translateX;
            m_ty += translateY;
            for (int column = 0; column < 2; column++) {
                for (int row = 0; row < 2; row++) {
                    result.m[row][column] = arm::smult16(other.m[0][column], m[row][0]) +
                                            arm::smult16(other.m[1][column], m[row][1]);
                }
            }
            for (int column = 0; column < 2; column++) {
                for (int row = 0; row < 2; row++) {
                    m[row][column] = result.m[row][column];
                }
            }
        }

        /** Append a rotation by `angle` (16.16 degrees). */
        void rotate(int angle)
        {
            int sine = math::CMathFixed::Sin(angle);
            int cosine = math::CMathFixed::Sin(angle + math::CMathFixed::Degrees_90);
            Matrix other(cosine, -sine, sine, cosine);
            Matrix result;
            for (int column = 0; column < 2; column++) {
                for (int row = 0; row < 2; row++) {
                    result.m[row][column] = arm::smult16(other.m[0][column], m[row][0]) +
                                            arm::smult16(other.m[1][column], m[row][1]);
                }
            }
            for (int column = 0; column < 2; column++) {
                for (int row = 0; row < 2; row++) {
                    m[row][column] = result.m[row][column];
                }
            }
        }

        int m[2][2];    // 0x00 16.16
        int m_tx;       // 0x10 16.16 translation
        int m_ty;       // 0x14
    };

    /** Bits of the transform flags (what the current transform does). */
    enum TransformFlag {
        TransformFlag_Translate = 1,
        TransformFlag_Scale = 2,
        TransformFlag_Rotate = 4
    };

    /** Two words the constructor clears (never used otherwise). */
    struct UnusedPair {
        UnusedPair()
        {
            m_second = 0;
            m_first = 0;
        }

        unsigned int m_first;
        unsigned int m_second;
    };

    /** Bytes of the draw instruction buffer (m_drawInstruction). */
    enum { DrawInstructionBufferSize = 0x64 };

    CGraphics2d_OGLES();
    virtual ~CGraphics2d_OGLES();

    virtual ICGraphicsAbstract::Abstraction GetAbstraction() const;
    virtual unsigned char Initialize();
    virtual void Destroy();
    virtual void LoadIdentity();
    virtual void LoadTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation);
    virtual void MultiplyTransform(const math::CMatrix2d* matrix, const math::CVector2d* translation);
    virtual void Translate(float x, float y);
    virtual void Rotate(float angle);
    virtual void Scale(float x, float y);
    virtual void PushTransform();
    virtual void PopTransform();
    virtual void GetTransform(const math::CMatrix2d** matrix, const math::CVector2d** translation);
    virtual void SetVertexPositionCoords(int count, unsigned int type, int stride, const void* coords);
    virtual void Draw(ICRenderSurface& surface, const components::CRectangle<short>* source,
                      const components::Color_RGBA* color, Flip flip);
    virtual void Draw(ICRenderSurface& surface, float x, float y, const components::CRectangle<short>* source,
                      const components::Color_RGBA* color, Flip flip);
    virtual void Draw(ICRenderSurface& surface, unsigned int width, unsigned int height,
                      const components::CRectangle<short>* source, const components::Color_RGBA* color, Flip flip);
    virtual void Draw(ICRenderSurface& surface, float x, float y, unsigned int width, unsigned int height,
                      const components::CRectangle<short>* source, const components::Color_RGBA* color, Flip flip);
    virtual void Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color);
    virtual void Draw(const components::CRectangle<short>& rectangle, const components::Color_RGBA& color,
                      float x, float y);
    virtual void Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color);
    virtual void Draw(const components::CRectangle<int>& rectangle, const components::Color_RGBA& color,
                      float x, float y);
    virtual void Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color);
    virtual void Draw(const components::CRectangle<float>& rectangle, const components::Color_RGBA& color,
                      float x, float y);
    virtual void Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color);
    virtual void Draw(const math::CLineSegment2d& line, const components::Color_RGBA& color, float x, float y);
    virtual void Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color);
    virtual void Draw(Primitive primitive, int first, int count, const components::Color_RGBA& color,
                      float x, float y);
    virtual void Draw(ICRenderText2d& text, const components::Color_RGBA* color);

    /** Pick the fixed function program among the device's intrinsic programs. */
    void BindIntrinsicShaderPrograms();
    /** Orthographic projection of the current target (pixels, y down), rotated with the device orientation. */
    void SetWidthAndHeightMappedOrthoProjection(math::CMatrix4d& matrix);
    /** Set up m_drawInstruction and the vertex layout fields for `vertexFormat` (VertexFormat). */
    void ConfigureVertexInfo(unsigned int vertexFormat);
    /** Draw lines between the coordinate pairs [first, first + count) of the position coordinates. */
    void DrawLines(int first, int count, const components::Color_RGBA& color);
    /** Fill the rectangles spanned by the coordinate pairs [first, first + count). */
    void DrawRectangles(int first, int count, const components::Color_RGBA& color);

    systems::CEventListener* m_eventListener;           // 0x08
    CGraphics_OGLES* m_graphics;                        // 0x0c the device
    CRasterizerState_v1* m_rasterizerState;             // 0x10 rasterizer state of the device
    unsigned int m_unused14;                            // 0x14
    ICShaderProgram* m_fixedFunctionProgram;            // 0x18 intrinsic program (OpenGL ES 1)
    ICShaderProgram* m_colorProgram;                    // 0x1c untextured, constant color
    ICShaderProgram* m_textureColorProgram;             // 0x20 textured, modulated by a constant color
    ICShaderProgram* m_textureProgram;                  // 0x24 textured
    ICShaderProgram* m_unusedProgram;                   // 0x28
    ICVertexBuffer* m_vertexBuffer2d;                   // 0x2c VertexFormat_Vtx2d, 10 vertices
    ICVertexBuffer* m_vertexBuffer2dTextured;           // 0x30 VertexFormat_Vtx2d_t
    ICVertexBuffer* m_vertexBuffer3d;                   // 0x34 VertexFormat_Vtx3d
    ICVertexBuffer* m_vertexBuffer3dTextured;           // 0x38 VertexFormat_Vtx3d_t
    ICIndexBuffer* m_indexBuffer;                       // 0x3c 30 unsigned shorts
    UnusedPair m_unused40;                              // 0x40
    UnusedPair m_unused48;                              // 0x48
    unsigned int m_unused50;                            // 0x50
    unsigned int m_vertexFormat;                        // 0x54 VertexFormat of the vertex fields below
    const CVertex::Attribute::Decl* m_vertexDecl;       // 0x58
    int m_positionComponents;                           // 0x5c 2 or 3 (with depth)
    int m_texCoordComponents;                           // 0x60 0 or 2
    int m_vertexSize;                                   // 0x64 bytes per vertex
    int m_vertexShorts;                                 // 0x68 shorts per vertex
    int m_texCoordOffset;                               // 0x6c shorts before the texture coordinates
    CDisplayProgram::Instruction::Draw* m_drawInstruction;  // 0x70 batch draw instruction being recorded
    int m_positionCount;                                // 0x74 components of the position coordinates
    unsigned int m_positionType;                        // 0x78 ComponentType of the position coordinates
    int m_positionStride;                               // 0x7c bytes between two coordinates
    const void* m_positionCoords;                       // 0x80 coordinates Draw(Primitive, ...) reads
    core::CStack<unsigned int> m_transformFlags;        // 0x84 TransformFlag bits per pushed transform
    unsigned char m_transformEnabled;                   // 0x90 the matrix stack is maintained
    core::CStack<Matrix> m_matrices;                    // 0x94
    math::CMatrix2dx m_fixedMatrix;                     // 0xa0 (unused)
    math::CVector2dx m_fixedTranslation;                // 0xb8 (unused)
    math::CMatrix2d m_matrix;                           // 0xc0 GetTransform() result
    math::CVector2d m_translation;                      // 0xd8 GetTransform() result
    int m_drawCount;                                    // 0xe0 surfaces drawn
};

} } } }

#endif
