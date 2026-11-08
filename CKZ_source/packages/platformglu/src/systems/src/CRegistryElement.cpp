// CRegistryElement.cpp - the element type CRegistry creates for registered objects.
#include "CRegistryElement.h"

#include <stddef.h>

namespace com { namespace glu { namespace platform { namespace systems {

CRegistryElement::CRegistryElement()
{
    m_classId = ClassId;
    m_isSentinel = 0;
    m_prev = NULL;
    m_next = NULL;
}

CRegistryElement::CRegistryElement(unsigned int handle, void* object, unsigned int elementClassId, int priority)
{
    m_classId = ClassId;
    m_isSentinel = 0;
    m_handle = handle;
    m_elementClassId = elementClassId;
    m_object = object;
    m_prev = NULL;
    m_next = NULL;
    m_priority = priority;
}

} } } }
