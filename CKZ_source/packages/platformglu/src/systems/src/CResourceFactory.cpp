// CResourceFactory.cpp - mime type -> CResource subclass.
#include "CResourceFactory.h"

#include "CResourceBigFile.h"
#include "CResourceBinary.h"
#include "CResourceDIB.h"
#include "CResourceFont.h"
#include "CResourceKeyset.h"
#include "CResourceMedia.h"
#include "CResourcePalette.h"
#include "CResourceRenderSurface.h"
#include "CResourceShader.h"
#include "CResourceShaderProgram.h"
#include "CResourceStrWChar.h"

namespace com { namespace glu { namespace platform { namespace systems {

unsigned char CResourceFactory::CreateResource(unsigned int mimeType, CResource** resource)
{
    *resource = CreateResource(mimeType);
    return *resource != NULL;
}

CResource* CResourceFactory::CreateResource(unsigned int mimeType)
{
    CResource* resource = NULL;
    switch (mimeType) {
    case 0xb52802e0:
    case 0xb52802fb:
    case 0xf1854bbc:
        resource = new CResourceShaderProgram();
        break;
    case 0x107dd145:
    case 0xb717945f:
        resource = new CResourceDIB();
        break;
    case 0x50055753:
    case 0x69e5abca:
    case 0xc5d54755:
        resource = new CResourceFont();
        break;
    case 0x593f8140:
    case 0x59efbc42:
    case 0x59f4bc42:
    case 0x59f4bc59:
    case 0x9f4bc472:
    case 0xe179efbe:
    case 0xe179f4be:
    case 0xfc31810d:
        resource = new CResourceRenderSurface();
        break;
    case 0x374f8659:
    case 0xf4e02223:
        resource = new CResourceBinary();
        break;
    case 0x43735eb4:
    case 0xf686aadc:
        resource = new CResourceStrWChar();
        break;
    case 0x2c43455a:
    case 0x2c43455c:
    case 0x2c435e5a:
    case 0x2c435e5c:
    case 0xcc4fcbf0:
    case 0xcc4fcbf6:
        resource = new CResourceShader();
        break;
    case 0x027b32f1:
    case 0x2fd9aa54:
    case 0x2fd9b34a:
    case 0x32fd8642:
    case 0x32fd99e1:
    case 0x32fd9f47:
    case 0xfd878582:
    case 0xfd8a7754:
    case 0xfd94b3c1:
        resource = new CResourceMedia();
        break;
    case 0x40546f85:
    case 0x40547da2:
    case 0xa6efb9a2:
        resource = new CResourcePalette();
        break;
    case 0x5ed4e9cb:
    case 0x69e4c505:
    case 0x74f30fa9:
        resource = new CResourceBigFile();
        break;
    case 0x69e5d35c:
        resource = new CResourceKeyset();
        break;
    }
    return resource;
}

} } } }
