// b074: complete surface-cache owner, original 0x119534..0x119a50.
#include "CBH_SurfaceManager.h"
#include "ICRenderSurface.h"
#include "ICGraphics.h"
#include "CInputStream.h"
#include "CResourceManager_v2.h"
#include "CApplet.h"
#include "CApp.h"

using namespace com::glu::platform::graphics;
using namespace com::glu::platform::components;
using namespace com::glu::platform::systems;

ICRenderSurface* CBH_SurfaceManager::CreateSurface(const char* name, bool keep, bool mipmaps)
{
    if (ImageInCache(name) == 1) {
        CachedSurface* cached = m_surfaces.Get(XString(name));
        ++cached->m_references;
        return cached->m_surface;
    }
    int abstraction = ICGraphics::GetInstance()->GetAbstraction();
    ICRenderSurface* surface = ICRenderSurface::CreateInstance(ICRenderSurface::Type_Default, ICRenderSurface::Targetability_0);
    CInputStream stream;
    unsigned int mimeType;
    const char* storedName;
    unsigned char nameDecompressed;
    ((CResourceManager_v2*)CApplet::m_pApp->m_resourceManager)->GetStream(name, stream, mimeType, NULL, &storedName, nameDecompressed);
    ICRenderSurface::InitParam parameters[6] = {
        {15, mimeType}, {14, (unsigned int)&stream}, {16, !mipmaps},
        {0, 0}, {0, 0}, {0, 0}
    };
    int count = 3;
    if (abstraction == 1) {
        parameters[count].m_key = 9;
        parameters[count].m_value = 0x7c1f;
        ++count;
    }
    if (mipmaps == 1) {
        parameters[count].m_key = 6;
        parameters[count].m_value = 0x21302;
    }
    surface->Initialize(parameters);
    if (keep == 1) {
        CachedSurface cached;
        cached.m_surface = surface;
        cached.m_name = name;
        cached.m_references = 1;
        m_surfaces.Add(XString(name), cached);
    }
    return surface;
}

bool CBH_SurfaceManager::ImageInCache(const char* name)
{
    if (name == NULL) {
        return false;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    return cached != NULL;
}

unsigned char CBH_SurfaceManager::IsImageExists(const char* name)
{
    if (ImageInCache(name)) {
        return 1;
    }
    CInputStream stream;
    unsigned int mimeType;
    const char* storedName;
    unsigned char nameDecompressed;
    return ((CResourceManager_v2*)CApplet::m_pApp->m_resourceManager)->GetStream(name, stream, mimeType, NULL, &storedName, nameDecompressed);
}

bool CBH_SurfaceManager::ReleaseSurface(const char* name)
{
    if (ImageInCache(name) != 1) {
        return false;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    --cached->m_references;
    if (cached->m_references != 0) {
        return false;
    }
    ICRenderSurface* surface = m_surfaces.Get(XString(name))->m_surface;
    delete surface;
    m_surfaces.Remove(XString(name));
    return true;
}

bool CBH_SurfaceManager::ReleaseSurface(ICRenderSurface* surface)
{
    for (int index = 0; index < m_surfaces.m_count; ++index) {
        XString* key = NULL;
        CachedSurface* cached = m_surfaces.GetAt(index, &key);
        if (cached->m_surface == surface) {
            XString::AnsiString name(key->m_data);
            return ReleaseSurface(name.m_text);
        }
    }
    return false;
}

unsigned int CBH_SurfaceManager::GetSurfaceWidth(ICRenderSurface* surface)
{
    if (surface == NULL) {
        return 0;
    }
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    return width;
}

unsigned int CBH_SurfaceManager::GetSurfaceHeight(ICRenderSurface* surface)
{
    unsigned int width;
    unsigned int height;
    surface->GetWidthAndHeight(width, height);
    return height;
}

void CBH_SurfaceManager::Clear()
{
    // The shipped owner clears entries only; surface lifetime is not changed here.
    m_surfaces.Clear();
}

ICRenderSurface* CBH_SurfaceManager::GetFromCache(const char* name)
{
    if (ImageInCache(name) != 1) {
        return NULL;
    }
    CachedSurface* cached = m_surfaces.Get(XString(name));
    return cached->m_surface;
}
