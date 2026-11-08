// Font resource creation and release, original 0x0dd538..0x0dd778.
#include "CFontMgr.h"
#include "CFont.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "CBinary.h"
#include "CArrayInputStream.h"
#include <string.h>

using namespace com::glu::platform;
using namespace com::glu::platform::components;
using namespace com::glu::platform::graphics;
using namespace com::glu::platform::systems;

CFontMgr::CFontMgr() : CSingleton(SingletonId)
{
    memset(m_fonts, 0, sizeof(m_fonts));
    memset(m_extraFonts, 0, sizeof(m_extraFonts));
}
CFontMgr::~CFontMgr()
{
    for (int index = 0; index < 31; ++index) {
        FreeFont(index);
    }
    for (int index = 0; index < 2; ++index) {
        if (m_extraFonts[index] != NULL) {
            delete m_extraFonts[index];
        }
    }
}
void CFontMgr::FreeFont(int index)
{
    if (m_fonts[index] == NULL) {
        return;
    }
    const GameFontInfo* info = &kTableGameFontInfo[index];
    if (info != NULL) {
        ((ICResourceManager*)CApplet::m_pApp->m_resourceManager)->ReleaseResource(info->m_image);
    }
    delete m_fonts[index];
    m_fonts[index] = NULL;
}
CFont* CFontMgr::GetFont(int index)
{
    CFont* result = m_fonts[index];
    if (result == NULL) {
        const GameFontInfo* info = &kTableGameFontInfo[index];
        if (info != NULL) {
            CFont* font = (CFont*)ICFont::CreateInstance(ICFont::Type_Bitmap);
            if (font != NULL) {
                ICResourceManager* resources = (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
                CResource* resource = NULL;
                resources->CreateResource(info->m_image, &resource);
                void* image = resource->GetData();
                resources->CreateResource(info->m_metrics, &resource);
                CBinary* binary = (CBinary*)resource->GetData();
                CArrayInputStream stream;
                stream.Open(binary->m_data, binary->m_size);
                ICFont::InitParam params[5];
                params[0].m_key = ICFont::InitParam_DataStream;
                params[0].m_value = (unsigned int)&stream;
                params[1].m_key = ICFont::InitParam_DataStreamType;
                params[1].m_value = 0xf4e02223;
                params[2].m_key = ICFont::InitParam_Image;
                params[2].m_value = (unsigned int)image;
                params[3].m_key = ICFont::InitParam_ImageType;
                params[3].m_value = 0xc9403f77;
                params[4].m_key = ICFont::InitParam_End;
                params[4].m_value = 0;
                font->Initialize(params);
                m_fonts[index] = font;
                resources->ReleaseResource(kTableGameFontInfo[index].m_metrics);
                result = font;
            }
        }
    }
    return result;
}
