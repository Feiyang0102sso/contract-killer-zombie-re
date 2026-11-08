// CKeysetResource.cpp - key list resource.
#include "CKeysetResource.h"
#include "CInputStream.h"
#include "CIdToObjectRouter.h"

namespace com { namespace glu { namespace platform { namespace components {

void CKeysetResource::Destroy()
{
    if (m_names != NULL) {
        for (unsigned int i = 0; i < m_count; i++) {
            if (m_names[i] != NULL) {
                free(m_names[i]);
                m_names[i] = NULL;
            }
        }
        delete[] m_names;
        m_names = NULL;
    }
    if (m_keys != NULL) {
        free(m_keys);
        m_keys = NULL;
    }
    m_count = 0;
}

unsigned char CKeysetResource::Load(CInputStream& stream, unsigned int type, CIdToObjectRouter* router)
{
    unsigned char result = 1;
    m_count = stream.ReadUInt16();
    if (m_count != 0) {
        m_keys = (unsigned int*)malloc(m_count * sizeof(unsigned int));
        if (router != NULL) {
            m_names = new char*[m_count];
        } else {
            m_names = NULL;
        }
        for (unsigned int i = 0; i < m_count; i++) {
            m_keys[i] = stream.ReadUInt32();
            if (m_names != NULL) {
                char* name = NULL;
                if (router != NULL) {
                    router->KeyToName(m_keys[i], &name);
                }
                m_names[i] = name;
            }
        }
        if (stream.m_error) {
            result = 0;
        }
    }
    return result;
}

} } } }
