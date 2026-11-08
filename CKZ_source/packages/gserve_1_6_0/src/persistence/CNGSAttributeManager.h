// CNGSAttributeManager.h - CAttributeManager: sends / queries avatar attributes (the profile data
// of attribute backed collections) and converts the server replies back into CNGSAttribute lists (b025x).
// Header-only singleton parts; the vtable and the inline destructors are emitted weak by every user.
#ifndef CNGSATTRIBUTEMANAGER_H
#define CNGSATTRIBUTEMANAGER_H

#include <stddef.h>
#include "CSingleton.h"
#include "CStringToKey.h"
#include "CVector.h"

class CNGSAttribute;
class CObjectMap;
class CObjectMapObject;
class CNGSServerRequestFunctor;
class CNGSUserCredentials;

/** Singleton key; every includer computes it at static initialization. */
static unsigned int ClassId_CAttributeManager = com::glu::platform::core::CStringToKey("CAttributeManager", 0);

class CAttributeManager : public com::glu::platform::components::CSingleton {
public:
    CAttributeManager()
        : CSingleton(ClassId_CAttributeManager)
    {
    }

    virtual ~CAttributeManager() {}

    /** The registered instance, created on first use. */
    static inline CAttributeManager* GetInstance()
    {
        void* found = NULL;
        CApplet::m_pApp->m_singletons->Find(ClassId_CAttributeManager, &found);
        CAttributeManager* instance = (CAttributeManager*)found;
        if (instance == NULL) {
            instance = new CAttributeManager();
        }
        return instance;
    }

    /** {"avatarattributes": [attributes...]}; the attributes become owned by the result. */
    static CObjectMapObject* createAttributeMessageObject(const com::glu::platform::core::CVector<CNGSAttribute*>& attributes);
    /** Send the attributes of the user; returns the message id. */
    unsigned int saveDataToServer(const com::glu::platform::core::CVector<CNGSAttribute*>& attributes,
                                  CNGSServerRequestFunctor* functor, const CNGSUserCredentials* credentials,
                                  unsigned char noSession);
    /** Ask for the attributes of the given collections; returns the message id. */
    unsigned int getDataFromServer(const com::glu::platform::core::CVector<int>& collectionTypes,
                                   CNGSServerRequestFunctor* functor, const CNGSUserCredentials* credentials);
    /** Attributes of a query reply ("list" array). */
    void extractAttributeValuesAsArray(CObjectMap* response, CNGSServerRequestFunctor* functor,
                                       com::glu::platform::core::CVector<CNGSAttribute*>& attributes);
    /**
     * Attributes of `root` (an "avatarattributes" array, or an object holding one unless `isList`);
     * 0 when anything could not be read.
     */
    unsigned char extractAttributeValuesAsArray(const CObjectMapObject* root,
                                                com::glu::platform::core::CVector<CNGSAttribute*>& attributes,
                                                bool isList);
    /** Attributes of `root` keyed by CStringToKey(name); returns their count. */
    int extractAttributeValuesAsHashMap(const CObjectMapObject* root, com::glu::platform::components::CHash& attributes);
};

#endif
