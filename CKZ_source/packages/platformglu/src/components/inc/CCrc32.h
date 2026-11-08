// CCrc32.h - CRC-32 (polynomial 0x04C11DB7, MSB first) with a lazily built table; an app singleton.
#ifndef CCRC32_H
#define CCRC32_H

#include "CSingleton.h"

namespace com { namespace glu { namespace platform { namespace components {

class CCrc32 : public CSingleton {
public:
    enum { SingletonId = 0x02744002 };

    CCrc32();
    virtual ~CCrc32();

    /** The registered instance, created on first use (inlined into gserve's persistence code). */  // added by b025x
    static inline CCrc32* GetInstance()  // added by b025x
    {  // added by b025x
        void* found = NULL;  // added by b025x
        CApplet::m_pApp->m_singletons->Find(SingletonId, &found);  // added by b025x
        CCrc32* instance = (CCrc32*)found;  // added by b025x
        if (instance == NULL) {  // added by b025x
            instance = new CCrc32();  // added by b025x
        }  // added by b025x
        return instance;  // added by b025x
    }  // added by b025x

    /** CRC of `size` bytes (0 for an empty block). */
    unsigned int Crc32(const unsigned char* data, unsigned int size);

    unsigned int m_table[256];      // 0x08 byte table; m_table[1] == 0 means "not built yet"
};

} } } }

#endif
