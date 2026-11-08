// CSwerve.cpp - application singleton owning the Swerve (M3G) runtime and its renderer.
#include "CSwerve.h"
#include "CSwerveGraphics3D.h"
#include "CSwerveLoader.h"
#include "ICGraphics.h"
#include "CGraphics_OGLES.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "ICStdUtil.h"
#include "ICDebug.h"

using namespace com::glu::platform;

CSwerve* CSwerve::m_pSwerve;

CSwerve::CSwerve() : components::CSingleton(SingletonId)
{
    m_swerve = NULL;
    m_graphics3D = NULL;
    Init();
}

unsigned char CSwerve::Init()
{
    // the runtime renders with OpenGL ES 1.x
    if (static_cast<graphics::CGraphics_OGLES*>(graphics::ICGraphics::GetInstance())->m_versionMajor != 1) {
        return 0;
    }
    int error = swvNew(&m_swerve);
    if (error == 0) {
        error = m_swerve->m_functions->Initialize(m_swerve);
        if (error == 0) {
            m_graphics3D = new CSwerveGraphics3D();
            if (m_graphics3D == NULL) {
                error = 2;
            }
        }
    }
    return error == 0;
}

CSwerve::~CSwerve()
{
    Free();
    m_pSwerve = NULL;
}

void CSwerve::Free()
{
    if (m_graphics3D != NULL) {
        delete m_graphics3D;
        m_graphics3D = NULL;
    }
    if (m_swerve != NULL) {
        m_swerve->m_functions->Release(m_swerve);
    }
    m_swerve = NULL;
}

void CSwerve::LogError(int error, const char* context)
{
    char number[16];
    switch (error) {
    case 1:
        malij297_ErrorMessageAppend("FAILED");
        break;
    case 2:
        malij297_ErrorMessageAppend("NOMEMORY");
        break;
    case 3:
        malij297_ErrorMessageAppend("CLASSNOTSUPPORT");
        break;
    case 0xe:
        malij297_ErrorMessageAppend("BADPARM");
        break;
    case 0x100:
        malij297_ErrorMessageAppend("FILEEXISTS");
        break;
    case 0x101:
        malij297_ErrorMessageAppend("FILENOEXISTS");
        break;
    case 0x102:
        malij297_ErrorMessageAppend("DIRNOTEMPTY");
        break;
    case 0x103:
        malij297_ErrorMessageAppend("BADFILENAME");
        break;
    case 0x104:
        malij297_ErrorMessageAppend("BADSEEKPOS");
        break;
    case 0x105:
        malij297_ErrorMessageAppend("FILEEOF");
        break;
    case 0x106:
        malij297_ErrorMessageAppend("FSFULL");
        break;
    case 0x107:
        malij297_ErrorMessageAppend("FILEOPEN");
        break;
    case 0x108:
        malij297_ErrorMessageAppend("OUTOFNODES");
        break;
    case 0x109:
        malij297_ErrorMessageAppend("DIRNOEXISTS");
        break;
    case 0x10a:
        malij297_ErrorMessageAppend("INVALIDOPERATION");
        break;
    case 0x10b:
        malij297_ErrorMessageAppend("NOMEDIA");
        break;
    // the runtime's own errors already wrote their text
    case 0x10000: case 0x10001: case 0x10002: case 0x10003: case 0x10004: case 0x10005: case 0x10006:
    case 0x10007: case 0x10008: case 0x10009: case 0x1000a: case 0x1000b: case 0x1000c: case 0x1000d:
    case 0x1000e: case 0x1000f: case 0x10010: case 0x10011: case 0x10012:
        break;
    default:
        core::ICStdUtil::SPrintF_S(number, sizeof(number), "%d)", error);
        malij297_ErrorMessageAppend(number);
        break;
    }
    core::ICDebug::LogMessage("SWERVE ERROR : %s%s", context, malij297_ErrorMessageGet());
    malij297_ErrorMessageReset();
}

IObject3D* CSwerve::LoadM3GResource(const char* name)
{
    systems::ICResourceManager* resourceManager = (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
    systems::CResource* resource = NULL;
    resourceManager->CreateResource(name, &resource);
    if (resource == NULL) {
        return NULL;
    }
    core::CVector<unsigned char>* data = (core::CVector<unsigned char>*)resource->GetData();
    const unsigned char* bytes = data->m_data;
    unsigned int size = data->m_size;
    core::CVector<IObject3D*> objects;
    IObject3D* root;
    int error = CSwerveLoader::load(size, bytes, 0, objects);
    if (error == 0) {
        // keep the first root, drop the others
        root = NULL;
        if (objects.m_size >= 1) {
            root = objects.m_data[0];
            for (int index = 1; index < objects.m_size; index++) {
                IObject3D* object = objects.m_data[index];
                object->m_functions->Release(object);
            }
        }
    } else {
        root = NULL;
        LogError(error, "CSwerve::LoadM3GResource");
    }
    resourceManager->ReleaseResource(name);
    return root;
}

int CSwerve::GenerateUserID(const char* text)
{
    int length = strlen(text);
    int hash = 0;
    if (length >= 1) {
        for (int index = length; index != 0; index--) {
            hash = hash * 1664525 + text[index - 1] + 1013904223;
        }
        hash = abs(hash) & 0x3fffffff;
    }
    return hash;
}
