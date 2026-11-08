// CSaveRestoreInterface.h - one collection of profile data that CProfileManager saves to disk and
// syncs with the server as CNGSAttribute lists (b025x). The game derives its data blocks from it.
#ifndef CSAVERESTOREINTERFACE_H
#define CSAVERESTOREINTERFACE_H

#include "CStrChar.h"
#include "CStrWChar.h"
#include "CVector.h"

class CNGSAttribute;

namespace com { namespace glu { namespace platform { namespace components {
class CHash;
} } } }

/** Save state of a collection. */
enum ProfileDataSaveStatus {
    PROFILE_DATA_NOT_LOADED = 0,
    PROFILE_DATA_SAVED_LOCAL = 1,       // saved on the device, not on the server yet
    PROFILE_DATA_SYNCED = 2,            // saved on the device and on the server
    PROFILE_DATA_STATUS_3 = 3,          // never set by gserve (meaning unknown)
    PROFILE_DATA_INVALIDATED = 4        // local copy must not be used
};

/** Where a collection lives on the server (getBackingStoreType()). */
enum ProfileDataBackingStore {
    BACKING_STORE_NONE = 0,
    BACKING_STORE_ATTRIBUTES = 1,       // sent as avatar attributes
    BACKING_STORE_CONTENT_FILE = 2      // uploaded as a content file; its descriptor holds the A/B name
};

/**
 * Abstract collection. No vtable is emitted anywhere in gserve (all virtual functions are pure and
 * there is no virtual destructor: CProfileManager frees collections with a plain delete).
 * Virtual slot names are inferred from the callers.
 */
class CSaveRestoreInterface {
public:
    /** +0x00 name of the collection: token prefix and file name. */
    virtual com::glu::platform::components::CStrChar getName() = 0;
    /** +0x04 1 when `data` (a saved file) has the expected format. */
    virtual unsigned char isDataFormatValid(const unsigned char* data, unsigned int size) = 0;
    /** +0x08 reset the game data of the collection (called by resetAllData()). */
    virtual void resetData() = 0;
    /** +0x0c never called by gserve (name unknown). */
    virtual void reservedVirtual0c() = 0;
    /** +0x10 copy the game data of `other` (called by copyAllData()). */
    virtual unsigned char copyData(const CSaveRestoreInterface& other) = 0;
    /** +0x14 append the game data as attributes. */
    virtual void getData(com::glu::platform::core::CVector<CNGSAttribute*>& attributes) = 0;
    /** +0x18 read the game data from attributes keyed by token name. */
    virtual void setData(const com::glu::platform::components::CHash* attributes) = 0;

    /** Copy the base fields and the game data of `other`; 1 when `other` is this object. */
    unsigned char copyAllData(const CSaveRestoreInterface& other);
    int getCollectionType();
    ProfileDataSaveStatus getDataSaveStatus();
    void setDataSaveStatus(ProfileDataSaveStatus status);
    /** Back to "not loaded" with the current version, then resetData(). */
    void resetAllData();
    /** The base fields as four int attributes followed by the game data. */
    void getAllData(com::glu::platform::core::CVector<CNGSAttribute*>& attributes);
    /** Wide version of getTokenNameFor(). */
    com::glu::platform::components::CStrWChar getwTokenNameFor(const char* name);
    /** Read the base fields and the game data back. */
    void setAllData(const com::glu::platform::components::CHash* attributes);
    /** "<getName()>_<name>". */
    com::glu::platform::components::CStrChar getTokenNameFor(const char* name);
    int getBackingStoreType();
    /** "<getName()>.dat". */
    com::glu::platform::components::CStrWChar getFilename();
    void setDescriptor(CSaveRestoreInterface* descriptor);
    CSaveRestoreInterface* getDescriptor();
    CSaveRestoreInterface* getFile();

    int m_currentVersion;                   // +0x04 version written by this build
    CSaveRestoreInterface* m_descriptor;    // +0x08 CProfileDataDescriptor of a content file collection
    CSaveRestoreInterface* m_file;          // +0x0c file collection described by this descriptor
    int m_collectionType;                   // +0x10
    ProfileDataSaveStatus m_dataSaveStatus; // +0x14
    int m_version;                          // +0x18 version of the loaded data
    int m_backingStoreType;                 // +0x1c ProfileDataBackingStore
};

#endif
