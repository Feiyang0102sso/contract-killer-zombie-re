// Complete original owner draft: five methods, 0x0c32e4..0x0c3330.
// Scratch only. Return-value promises are limited to what original callers prove.
#include "CApplet.h"
#include "ICCore.h"
#include "NPMalloc.h"
#include "NPMem.h"
#include "SwerveApi.h"

extern "C" {

void* malij297_Allocate(size_t size)
{
    return np_malloc(size);
}

void malij297_Free(void* pointer)
{
    np_free(pointer);
}

void malij297_MemSet(void* destination, int value, size_t size)
{
    np_memset(destination, static_cast<unsigned char>(value), size); // Original c3300 masks a full argument word.
}

void malij297_MemCpy(void* destination, const void* source, size_t size)
{
    np_memcpy(destination, source, size);
}

void malij297_Panic()
{
    // Original c3328/c332a: ICCore slot +0xc, status word 1; no invented status name.
    CApplet::m_pCore->ExitApp(static_cast<com::glu::platform::framework::ICCore::eExitStatus>(1));
}

}

// This file is installed at the original owner path as the recovered production source; the "Scratch only" note above describes the draft stage in the evidence directory.
