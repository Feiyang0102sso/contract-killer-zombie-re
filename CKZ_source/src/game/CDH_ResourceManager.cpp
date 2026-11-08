// CDH_ResourceManager.cpp - game resources: image surfaces cached by name, other data from the
// platform resource manager.
#include "CDH_ResourceManager.h"
#include <string.h>
#include "CApplet.h"
#include "CApp.h"
#include "CInputStream.h"
#include "CResource.h"
#include "CResourceManager_v2.h"
#include "ICGraphics.h"
#include "ICRenderSurface.h"
#include "App.h"
#include "DGHelper.h"

using namespace com::glu::platform;
using com::glu::platform::graphics::ICRenderSurface;

/** Mime type keys of the image resources (loaded as surfaces). */
static const unsigned int MIME_IMAGE_1 = 0xb7178678;
static const unsigned int MIME_IMAGE_2 = 0xb717945f;
static const unsigned int MIME_IMAGE_3 = 0x7179dacc;
/** Color made transparent on abstraction 1. */
static const unsigned int IMAGE_COLOR_KEY = 0xff00ff;

/** The platform resource manager of the application. */
static inline systems::CResourceManager_v2* PlatformResources()
{
    return (systems::CResourceManager_v2*)CApplet::m_pApp->m_resourceManager;
}

CDH_ResourceManager::CDH_ResourceManager()
{
}

CDH_ResourceManager::~CDH_ResourceManager()
{
    flush();
}

void CDH_ResourceManager::flush()
{
    for (int index = 0; index < m_images.size(); index++) {
        if (m_images[index].m_surface != NULL) {
            delete m_images[index].m_surface;
            m_images[index].m_surface = NULL;
        }
    }
    m_images.removeAllElements();
}

bool CDH_ResourceManager::releaseImage(const char* name)
{
    for (int index = 0; index < m_images.size(); index++) {
        // Same NULL rules as CStrChar::operator==.
        const char* imageName = m_images[index].m_name.GetCharPtr();
        bool same;
        if (imageName != NULL && name != NULL) {
            same = strcmp(imageName, name) == 0;
        } else {
            same = imageName == name;
        }
        if (same) {
            if (m_images[index].m_surface != NULL) {
                delete m_images[index].m_surface;
                m_images[index].m_surface = NULL;
            }
            m_images.removeElementAt(index);
            return true;
        }
    }
    return false;
}

ICRenderSurface* CDH_ResourceManager::createImageSurface(const char* name, int orientation)
{
    ICRenderSurface* surface = getFromCache(name);
    if (surface != NULL) {
        return surface;
    }

    int abstraction = graphics::ICGraphics::GetInstance()->GetAbstraction();
    surface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Default, ICRenderSurface::Targetability_0);

    systems::CSystemElement* element;
    components::CInputStream stream;
    unsigned int mimeType;
    unsigned char nameDecompressed;
    PlatformResources()->GetStream(name, stream, mimeType, &element, NULL, nameDecompressed);

    ICRenderSurface::InitParam params[6];
    params[0].m_key = ICRenderSurface::InitParam_SourceType;
    params[0].m_value = mimeType;
    params[1].m_key = ICRenderSurface::InitParam_SourceStream;
    params[1].m_value = (unsigned int)&stream;
    params[2].m_key = ICRenderSurface::InitParam_HighColor;
    params[2].m_value = 1;
    params[3].m_key = ICRenderSurface::InitParam_End;
    params[3].m_value = 0;
    params[4].m_key = ICRenderSurface::InitParam_End;
    params[4].m_value = 0;
    params[5].m_key = ICRenderSurface::InitParam_End;
    params[5].m_value = 0;
    int count = 3;
    if (orientation >= 0) {
        params[3].m_key = ICRenderSurface::InitParam_Orientation;
        params[3].m_value = orientation;
        count = 4;
    }
    if (abstraction == 1) {
        params[count].m_key = ICRenderSurface::InitParam_ColorKey;
        params[count].m_value = IMAGE_COLOR_KEY;
    }
    surface->Initialize(params);

    TImage image;
    image.m_name = name;
    image.m_orientation = orientation;
    image.m_surface = surface;
    m_images.addElement(image);
    return surface;
}

ICRenderSurface* CDH_ResourceManager::getFromCache(const char* name)
{
    for (int index = 0; index < m_images.size(); index++) {
        if (strcmp(m_images[index].m_name.GetCharPtr(), name) == 0) {
            return m_images[index].m_surface;
        }
    }
    return NULL;
}

void* CDH_ResourceManager::getResourceData(const char* name, bool loading)
{
    void* data = getFromCache(name);
    if (data != NULL) {
        return data;
    }

    ((App*)WindowApp::m_instance)->m_helper->m_loadingResource = loading;
    systems::CSystemElement* element;
    components::CInputStream stream;
    unsigned int mimeType;
    unsigned char nameDecompressed;
    PlatformResources()->GetStream(name, stream, mimeType, &element, NULL, nameDecompressed);
    if (mimeType == MIME_IMAGE_1 || mimeType == MIME_IMAGE_2 || mimeType == MIME_IMAGE_3) {
        data = createImageSurface(name, -1);
    } else {
        data = NULL;
        systems::CResource* resource;
        PlatformResources()->CreateResource(name, &resource);
        if (resource != NULL) {
            data = resource->GetData();
        }
    }
    return data;
}

void CDH_ResourceManager::releaseResource(const char* name)
{
    if (!releaseImage(name)) {
        PlatformResources()->ReleaseResource(name);
    }
}
