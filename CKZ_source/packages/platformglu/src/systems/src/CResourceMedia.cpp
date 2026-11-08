// CResourceMedia.cpp - resource wrapping a components::CMedia.
#include "CResourceMedia.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

namespace com { namespace glu { namespace platform { namespace systems {

CResourceMedia::CResourceMedia()
{
    m_classId = ClassId;
    m_media = NULL;
    CResource::ResetMembers();
}

CResourceMedia::~CResourceMedia()
{
    CResourceMedia::Destroy();
}

void CResourceMedia::Destroy()
{
    if (m_media != NULL) {
        delete m_media;
        m_media = NULL;
    }
}

void CResourceMedia::ResetMembers()
{
    CResource::ResetMembers();
}

unsigned int CResourceMedia::GetSize() const
{
    if (m_media == NULL) {
        return 0;
    }
    return m_media->m_size + sizeof(components::CMedia);
}

void* CResourceMedia::GetData() const
{
    return m_media;
}

unsigned int CResourceMedia::CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                            unsigned int mimeType, components::CIdToObjectRouter* router,
                                            unsigned char copyName)
{
    m_handle = handle;
    m_media = new components::CMedia();
    unsigned int result = CreateResult_Created;
    if (stream != NULL) {
        wchar_t mediaName[0x100];
        mediaName[0] = 0;
        if (name != NULL && strlen(name) <= 0xff) {
            mbstowcs(mediaName, name, strlen(name) + 1);
        } else {
            // no usable name: use the handle as 8 hex digits
            wchar_t* digit = &mediaName[7];
            for (int shift = 0; shift < 32; shift += 4) {
                int nibble = (handle >> shift) & 0xf;
                if (nibble >= 10) {
                    *digit = 'a' + (char)(nibble - 10);
                } else {
                    *digit = '0' | nibble;
                }
                digit--;
            }
            mediaName[8] = 0;
        }
        // ?: kept on purpose: the original selects the result instead of branching
        result = m_media->Load(*stream, mimeType, mediaName) ? CreateResult_Loaded : CreateResult_Failed;
    }
    if (result != CreateResult_Failed) {
        SetName(name, copyName);
        m_isCreated = 1;
    } else {
        Destroy();
        result = CreateResult_Failed;
    }
    return result;
}

} } } }
