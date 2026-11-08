// CResource.cpp - base of the managed resources.
#include "CResource.h"

#include <string.h>
#include <stdlib.h>
#include "ICResourceManager.h"

namespace com { namespace glu { namespace platform { namespace systems {

CResource::CResource()
{
    m_classId = ClassId;
    m_source = NULL;
    m_name = NULL;
    m_nextSameHandle = NULL;
    m_handle = 0;
    m_refCount = 0;
    m_isCreated = 0;
    m_ownsName = 0;
    m_manager = NULL;
}

CResource::~CResource()
{
    if (m_manager != NULL) {
        m_manager->RemoveResource(*this);
    }
    DestroyName();
}

void CResource::DestroyName()
{
    if (m_ownsName && m_name != NULL) {
        free(m_name);
        m_ownsName = 0;
    }
    m_name = NULL;
}

void CResource::SetName(const char* name, unsigned char copy)
{
    if (!copy) {
        DestroyName();
    } else {
        unsigned int size = strlen(name) + 1;
        char* nameCopy = (char*)malloc(size);
        memcpy(nameCopy, name, size);
        DestroyName();
        if (nameCopy != NULL) {
            m_name = nameCopy;
            m_ownsName = 1;
            return;
        }
        // out of memory: fall back to the caller's string
    }
    m_name = (char*)name;
}

} } } }
