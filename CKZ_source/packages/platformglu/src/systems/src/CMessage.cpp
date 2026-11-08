// CMessage.cpp - events carrying typed arguments.
#include "CMessage.h"

#include <stdarg.h>

namespace com { namespace glu { namespace platform { namespace systems {

CMessage::CMessage(core::CClass* sender, unsigned int messageId, unsigned int param, int argumentCount, ...)
{
    m_classId = ClassId;
    m_argumentCount = argumentCount;
    m_eventId = messageId;
    m_sender = sender;
    m_param = param;

    va_list arguments;
    va_start(arguments, argumentCount);
    for (int index = 0; index < argumentCount; index++) {
        m_arguments[index].m_type = va_arg(arguments, unsigned int);
        m_arguments[index].m_count = va_arg(arguments, unsigned int);
        m_arguments[index].m_data = va_arg(arguments, void*);
    }
    va_end(arguments);
}

} } } }
