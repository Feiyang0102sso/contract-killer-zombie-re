// CInputMapping.cpp - base of the input mappings.
#include "CInputMapping.h"

namespace com { namespace glu { namespace platform { namespace systems {

CInputMapping::CInputMapping() : components::CExecutable(InfinitelyRunning)
{
    m_classId = ClassId;
    m_targetHandle = 0;
}

CInputMapping::~CInputMapping()
{
}

} } } }
