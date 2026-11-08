// Complete original owner draft: malij297_error.cpp, 0x0c0e98..0x0c0fc0.
// Scratch only; no production source or shared declarations are changed here.
#include "CSwerve.h"
#include "SwerveApi.h"

extern "C" {

int malij297_ErrorMessageBufferInit()
{
    malij297_ErrorMessageReset();
    return 1; // Original c0ea0 writes the full return register to one.
}

void malij297_ErrorMessageReset()
{
    CSwerve* swerve = CSwerve::GetInstance();
    swerve->m_buffer.m_length = 0;
    swerve->m_buffer.m_string[0] = 0;
}

void malij297_ErrorMessageBufferDestroy()
{
    // Original c0efc is bx lr followed by alignment nop; it owns nothing.
}

const char* malij297_ErrorMessageGet()
{
    return CSwerve::GetInstance()->m_buffer.GetCharPtr();
}

void malij297_ErrorMessageAppend(const char* message)
{
    com::glu::platform::components::CStrCharBuffer& buffer = CSwerve::GetInstance()->m_buffer;
    buffer.Append(" : ");
    buffer.Append(message);
}

}

// This file is installed at the original owner path as the recovered production source; the "Scratch only" note above describes the draft stage in the evidence directory.
