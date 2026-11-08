// ICDisplayProgram.h - recorded list of device instructions, executed later (or kept and replayed).
// (first drafted by b002; completed by b037x, owner of CDisplayProgram.cpp where Mode is implemented)
// Slot order from the vtables of CDisplayProgram / CDisplayProgram_OGLES.
#ifndef ICDISPLAYPROGRAM_H
#define ICDISPLAYPROGRAM_H

#include "ICGraphicsResource.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class ICDisplayProgram : public ICGraphicsResource {
public:
    /** Resource type key of display programs. */
    enum { ResourceType = 0x106742cf };

    /** One (key, value) initialization setting; a list ends with key 0. */
    struct InitParam {
        /** Keys (a value of 0 keeps the default of ConfigDefaultAllocationInfo()). */
        enum Key {
            Key_End = 0,
            Key_IndexBufferLength = 1,      // indices per shared index buffer
            Key_VertexBufferLength = 2,     // vertices per shared vertex buffer
            Key_IndexStreamLength = 3,      // indices reserved per recorded index stream
            Key_VertexStreamLength = 4      // vertices reserved per recorded vertex stream
        };

        unsigned int m_key;     // 0x00
        unsigned int m_value;   // 0x04
    };

    /** What the program is (GetDescription). */
    struct Description {
        unsigned int m_type;    // 0x00
    };

    /**
     * How the instructions of a program are sorted into levels while recording. The configure
     * callback lays out the levels of the program and copies the sorting actions below into it.
     */
    class Mode {
    public:
        /** Built-in recording modes. */
        enum Intrinsic {
            BlendBiased = 0,
            BlendBiasedIgnoreTargetCommands = 1,
            SortAllBackToFront = 2,
            SortAllBackToFrontIgnoreTargetCommands = 3,
            SortAllBackToFrontIgnoreTargetColorMaskBackfaceCullingAndDepthCommands = 4
        };

        /** Lays out the levels of `program` for this mode. */
        typedef void (*ConfigureFunc)(void* context, ICDisplayProgram& program);

        Mode(Intrinsic intrinsic);

        /** Switch to one of the built-in modes (unknown values leave the mode untouched). */
        void Set(Intrinsic intrinsic);

        void* m_context;                    // 0x00 first argument of m_configure (the mode itself)
        ConfigureFunc m_configure;          // 0x04
        unsigned char m_blendBiased;        // 0x08 blending instructions start a new batch
        unsigned int m_targetAction;        // 0x0c how target instructions are merged
        unsigned int m_stateActionSorted;   // 0x10 state instruction action when sorting by layer
        unsigned int m_stateAction;         // 0x14 state instruction action otherwise
        unsigned int m_drawActionSorted;    // 0x18 draw instruction action when sorting by layer
        unsigned int m_drawAction;          // 0x1c draw instruction action otherwise
    };

    /** Display program of the current abstraction (NULL when there is none). */
    static ICDisplayProgram* CreateInstance();

    ICDisplayProgram() : ICGraphicsResource(ResourceType) {}

    virtual ~ICDisplayProgram() {}

    virtual unsigned char Initialize(const InitParam* params) = 0;
    virtual void Destroy() = 0;
    /** Layer that following instructions are sorted by. */
    virtual void SetLayer(int layer) = 0;
    virtual int GetLayer() = 0;
    /** `callback(context, instruction)` executes the instructions of `level`. */
    virtual void SetCallBackInstructionExecution(unsigned int level, void* context,
                                                 void (*callback)(void* context, unsigned char* instruction)) = 0;
    /** Execute all recorded instructions. */
    virtual void Run() = 0;
    virtual unsigned char GetDescription(Description& description) = 0;
    /** Record a copy of `instruction` (and of the instructions pending in the levels above it). */
    virtual void Add(unsigned char* instruction) = 0;
};

} } } }

#endif
