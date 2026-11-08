// CResourceBigFile.h - resource wrapping a components::CBigFile. (created by b011x)
#ifndef CRESOURCEBIGFILE_H
#define CRESOURCEBIGFILE_H

#include "CResource.h"
#include "CBigFile_v2.h"

namespace com { namespace glu { namespace platform { namespace systems {

class CResourceBigFile : public CResource {
public:
    /** Runtime class id: CStringToKey("CResourceBigFile"). */
    enum { ClassId = 0x70d68df4 };

    CResourceBigFile();
    virtual ~CResourceBigFile();

    virtual void Destroy();
    virtual unsigned int GetSize() const;
    virtual void* GetData() const;
    virtual unsigned int CreateInternal(unsigned int handle, const char* name, components::CInputStream* stream,
                                        unsigned int mimeType, components::CIdToObjectRouter* router,
                                        unsigned char copyName);
    virtual void ResetMembers();

    components::CBigFile* m_bigFile; // 0x24
};

} } } }

#endif
