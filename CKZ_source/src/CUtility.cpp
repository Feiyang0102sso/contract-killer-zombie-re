// Localized strings and exact UI input-map registrations, original 0x0e1604..0x0e1aac.
#include "CUtility.h"
#include "CApplet.h"
#include "CApp.h"
#include "CAppProperties.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "CKeyInputMapping.h"
#include "CStrWChar.h"

using namespace com::glu::platform;
using namespace com::glu::platform::components;
using namespace com::glu::platform::systems;

namespace com { namespace glu { namespace platform { namespace components {
extern const char JADKEY__SOFTKEY_REVERSE[];
} } } }

void CUtility::GetString(CStrWChar& text, const char* id)
{
    text.ReleaseMemory();
    text.Concatenate("");
    AppendString(text, id);
}
void CUtility::AppendString(CStrWChar& text, const char* id)
{
    ICResourceManager* resources = (ICResourceManager*)CApplet::m_pApp->m_resourceManager;
    CResource* resource = NULL;
    resources->CreateResource(id, &resource);
    if (resource != NULL) {
        CStrWChar* value = (CStrWChar*)resource->GetData();
        if (value != NULL) {
            text.Concatenate(value->GetWCharPtr());
        }
        resources->ReleaseResource(id);
    }
}
CStrWChar CUtility::GetString(const char* id)
{
    CStrWChar text("");
    AppendString(text, id);
    return text;
}
void CUtility::RegisterUIKeys()
{
    CKeyInputMapping::Register(0x92484fb2, 0x3f972120, (core::eKeyCode)4, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x972028c1, (core::eKeyCode)5, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x9720a3cb, (core::eKeyCode)6, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x7214ff0d, (core::eKeyCode)7, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x2074d009, (core::eKeyCode)11, 1, 0, 0, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x97204784, (core::eKeyCode)12, 1, 0, 0, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x9720b34a, (core::eKeyCode)0x8c, 1, 0, 0, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x97200e9a, (core::eKeyCode)0x20, 1, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0xfd4836a5, (core::eKeyCode)0x20, 4, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x51212dd9, (core::eKeyCode)0x6b, 4, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x3f972120, (core::eKeyCode)0x32, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x972028c1, (core::eKeyCode)0x38, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x9720a3cb, (core::eKeyCode)0x34, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x7214ff0d, (core::eKeyCode)0x36, 3, 500, 500, 1);
    CKeyInputMapping::Register(0x92484fb2, 0x2074d009, (core::eKeyCode)0x35, 1, 0, 0, 1);
}
void CUtility::RegisterUISoftkeys()
{
    void* found = NULL;
    CApplet::m_pApp->m_singletons->Find(CAppProperties::SingletonId, &found);
    CAppProperties* instance = (CAppProperties*)found;
    if (instance == NULL) {
        instance = new CAppProperties();
    }
    unsigned int rightAction;
    if (instance->m_properties.GetBool(JADKEY__SOFTKEY_REVERSE) == 0) {
        CKeyInputMapping::Register(0x92484fb2, 0x5f61a84a, (core::eKeyCode)13, 1, 0, 0, 1);
        rightAction = 0xd45f52de;
    } else {
        CKeyInputMapping::Register(0x92484fb2, 0xd45f52de, (core::eKeyCode)13, 1, 0, 0, 1);
        rightAction = 0x5f61a84a;
    }
    CKeyInputMapping::Register(0x92484fb2, rightAction, (core::eKeyCode)14, 1, 0, 0, 1);
}
void CUtility::UnregisterUIKeys()
{
    // The original repeats the first five actions and does not unregister the release mappings here.
    CKeyInputMapping::Unregister(0x92484fb2, 0x3f972120, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x972028c1, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x9720a3cb, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x7214ff0d, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x2074d009, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x97204784, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x9720b34a, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x3f972120, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x972028c1, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x9720a3cb, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x7214ff0d, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0x2074d009, (core::eKeyCode)0);
}
void CUtility::UnregisterUISoftkeys()
{
    CKeyInputMapping::Unregister(0x92484fb2, 0x5f61a84a, (core::eKeyCode)0);
    CKeyInputMapping::Unregister(0x92484fb2, 0xd45f52de, (core::eKeyCode)0);
}
