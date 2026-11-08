// CVertex.h - vertex layout description: attribute ids, attribute declarations and the named
// declaration list a vertex buffer keeps.
// (first drafted by b037x as PROVISIONAL; completed by b040x, the owner of CVertex.cpp)
#ifndef CVERTEX_H
#define CVERTEX_H

#include "CStrWChar.h"
#include "CVector.h"

namespace com { namespace glu { namespace platform { namespace graphics {

class CVertex {
public:
    class Attribute {
    public:
        class Id {
        public:
            /** Intrinsic attribute ids (GetInstrinsicName); bit 16 marks an intrinsic id. */
            enum Intrinsic {
                Color       = 0x10000,
                Depth       = 0x10001,
                Binormal    = 0x10002,
                BlendIndics = 0x10003,
                BlendWeights = 0x10004,
                Fog         = 0x10005,
                Normal      = 0x10006,
                PointSize   = 0x10007,
                Position    = 0x10008,
                Position1   = 0x10009,
                PositionT   = 0x1000a,
                Sample      = 0x1000b,
                Tangent     = 0x1000c,
                TessFactor  = 0x1000d,
                TexCoord    = 0x1000e,
                TexCoord1   = 0x1000f,
                TexCoord2   = 0x10010,
                TexCoord3   = 0x10011,
                TexCoord4   = 0x10012,
                TexCoord5   = 0x10013,
                TexCoord6   = 0x10014,
                TexCoord7   = 0x10015,
                End         = 0xffffffff    // terminates a Decl list
            };

            /** Flag set in every intrinsic id. */
            enum { IntrinsicFlag = 0x10000 };

            /** Name of an intrinsic id ("Position", ...), NULL for other ids. */
            static const wchar_t* GetInstrinsicName(unsigned int id);
        };

        /** One attribute of a vertex; arrays of these end with id Id::End. */
        struct Decl {
            unsigned int m_id;              // 0x00 Id
            unsigned int m_type;            // 0x04 ComponentType
            unsigned int m_offset;          // 0x08 byte offset inside the vertex
            short m_count;                  // 0x0c components (signed: CVertexBuffer::Initialize uses ldrsh)
            short m_stride;                 // 0x0e bytes per vertex
        };
    };

    /** The attribute list of a vertex buffer. */
    class Decl {
    public:
        /** An attribute together with its name. */
        struct NamedAttributeDecl {
            components::CStrWChar m_name;   // 0x00
            Attribute::Decl m_decl;         // 0x10

            // user-declared: CVector copies elements without zero-filling the slot first
            NamedAttributeDecl()
            {
            }
        };

        /** Room for 5 attributes, growing by 5. */
        Decl() : m_attributes(5, 5)
        {
        }

        /**
         * Replace the list with the attributes of `decls` (terminated by Id::End). With
         * `useIntrinsicNames` the intrinsic attributes are named after their id.
         */
        void Set(const Attribute::Decl* decls, unsigned char useIntrinsicNames);

        core::CVector<NamedAttributeDecl> m_attributes;    // 0x00
    };
};

// Defined in the header (weak in every user that calls it).
inline void CVertex::Decl::Set(const Attribute::Decl* decls, unsigned char useIntrinsicNames)
{
    m_attributes.Clear();
    while (decls->m_id != Attribute::Id::End) {
        NamedAttributeDecl named;
        named.m_decl = *decls;
        if (useIntrinsicNames && (named.m_decl.m_id & Attribute::Id::IntrinsicFlag)) {
            named.m_name = Attribute::Id::GetInstrinsicName(named.m_decl.m_id);
        }
        m_attributes.Add(named);
        decls++;
    }
}

} } } }

#endif
