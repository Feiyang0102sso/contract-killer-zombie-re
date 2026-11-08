// CProfileManager.cpp - player profile load / save / server sync (b025x).
#include "CProfileManager.h"
#include "CProfileDataDescriptor.h"
#include "CNGSAttribute.h"
#include "CNGSAttributeManager.h"
#include "CNGSContentManager.h"
#include "CNGS.h"
#include "CNGSLocalUser.h"
#include "CNGSSession.h"
#include "CNGSUtil.h"
#include "CNGSJSONParser.h"
#include "CNGSFunctor.h"
#include "CNGSFileMgr.h"
#include "CFileUtil_gServe.h"
#include "CArrayStream_gServe.h"
#include "CNotificationHandler.h"
#include "CObjectMapObject.h"
#include "CHash.h"
#include "CCrc32.h"
#include "CUtf.h"

using com::glu::platform::components::CStrWChar;
using com::glu::platform::components::CHash;
using com::glu::platform::components::CCrc32;
using com::glu::platform::core::CVector;
using com::glu::platform::core::CUtf;
using com::glu::platform::network::sDataBlock;

/** Key of the obfuscated profile files. */
static const wchar_t* const PROFILE_FILE_KEY = L"GgLlUu";

CProfileManager::CProfileManager(const CNGSUserCredentials* credentials)
{
    m_obfuscateFiles = false;
}

CProfileManager::~CProfileManager()
{
    for (int index = 0; index < m_registeredData.Size(); index++) {
        if (m_registeredData[index] != NULL) {
            delete m_registeredData[index];
            m_registeredData[index] = NULL;
        }
    }
    m_registeredData.Clear();
}

void CProfileManager::getAllRegisteredCollections(CVector<int>& collectionTypes)
{
    for (int collectionType = m_firstCollectionType; collectionType <= m_lastCollectionType; collectionType++) {
        collectionTypes.Add(collectionType);
    }
}

CSaveRestoreInterface* CProfileManager::getGameData(int collectionType, GameDataSource source)
{
    // Descriptor types live below the registered range and map onto their file collection.
    int index = collectionType;
    if (collectionType < m_firstCollectionType) {
        index = CProfileDataDescriptor::targetedCollectionType(collectionType);
    }
    CSaveRestoreInterface* data = NULL;
    switch (source) {
    case GAME_DATA_LOCAL:
        data = m_localData[index - m_firstCollectionType];
        break;
    case GAME_DATA_SERVER:
        data = m_serverData[index - m_firstCollectionType];
        break;
    case GAME_DATA_CURRENT:
        data = m_currentData[index - m_firstCollectionType];
        break;
    }
    if (collectionType < m_firstCollectionType) {
        data = data->getDescriptor();
    }
    return data;
}

void CProfileManager::loadSaveStart()
{
    CNGSContentManager::GetInstance()->Lock();
}

unsigned char CProfileManager::loadFromFileSystem(int collectionType, int userId)
{
    CSaveRestoreInterface* data = getGameData(collectionType, GAME_DATA_LOCAL);
    if (data != NULL) {
        data->resetAllData();
    }
    unsigned int size = CFileUtil_gServe::GetFilesize(CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr(),
                                                      data->getFilename().GetWCharPtr());
    if (size == 0) {
        return 0;
    }
    unsigned char loaded = 0;
    unsigned char* buffer = new unsigned char[size];
    memset(buffer, 0, size);
    unsigned char read = CFileUtil_gServe::ReadApplicationDataFile(data->getFilename().GetWCharPtr(), buffer, size,
                                                                   CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr());
    if (read && !data->isDataFormatValid(buffer, size)) {
        // Let the game convert an old file format.
        unsigned char* convertedData = NULL;
        size = 0;
        CNotificationHandler* handler = CNGS::GetInstance()->GetLocalUser()->m_notificationHandler;
        if (!handler->HandleProfileDataFormatIncorrect(data->getFilename(), CNGSUserCredentials::getSubDirectoryPath(userId),
                                                       convertedData, size)) {
            read = 0;
        } else {
            if (buffer != NULL) {
                delete[] buffer;
            }
            buffer = convertedData;
        }
    }
    if (read) {
        if (ReadRawFileDataToAttributes(buffer, size, data)) {
            loaded = 1;
            if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                CSaveRestoreInterface* descriptor = data->getDescriptor();
                if (descriptor != NULL) {
                    size = CFileUtil_gServe::GetFilesize(CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr(),
                                                         descriptor->getFilename().GetWCharPtr());
                    if (size == 0) {
                        loaded = 0;
                        descriptor->resetAllData();
                    } else {
                        if (buffer != NULL) {
                            delete[] buffer;
                        }
                        buffer = new unsigned char[size];
                        loaded = 0;
                        memset(buffer, 0, size);
                        unsigned char descriptorRead =
                            CFileUtil_gServe::ReadApplicationDataFile(descriptor->getFilename().GetWCharPtr(), buffer, size,
                                                                      CNGSUserCredentials::getSubDirectoryPath(userId).GetWCharPtr());
                        descriptor->resetAllData();
                        if (descriptorRead && ReadRawFileDataToAttributes(buffer, size, descriptor)) {
                            loaded = 1;
                        }
                    }
                }
            }
        }
    }
    if (buffer != NULL) {
        delete[] buffer;
    }
    return loaded;
}

void CProfileManager::copyDataBuffer(const CVector<int>& collectionTypes, CVector<CSaveRestoreInterface*>* destination,
                                     CVector<CSaveRestoreInterface*>* source)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        int slot = collectionTypes[index];
        if (collectionTypes[index] < m_firstCollectionType) {
            slot = CProfileDataDescriptor::targetedCollectionType(collectionTypes[index]);
        }
        CSaveRestoreInterface* from = (*source)[slot - m_firstCollectionType];
        CSaveRestoreInterface* to = (*destination)[slot - m_firstCollectionType];
        if (collectionTypes[index] < m_firstCollectionType) {
            // A descriptor: copy its file collection and continue with the descriptors.
            to->copyAllData(*from);
            from = from->getDescriptor();
            to = to->getDescriptor();
        }
        if (from->getDescriptor() != NULL && to->getDescriptor() != NULL) {
            to->getDescriptor()->copyAllData(*from->getDescriptor());
        }
        to->copyAllData(*from);
    }
}

void CProfileManager::loadSaveEnd()
{
    CNGSContentManager::GetInstance()->Unlock();
}

/** Delete the attributes of a CHash filled by CAttributeManager::extractAttributeValuesAsHashMap. */
static inline void DeleteAttributes(CHash& attributes)
{
    for (int bucket = 0; bucket < attributes.m_bucketCount; bucket++) {
        for (CHash::Entry* entry = attributes.m_buckets[bucket]; entry != NULL; entry = entry->m_next) {
            CNGSAttribute* attribute = (CNGSAttribute*)entry->m_value;
            if (attribute != NULL) {
                delete attribute;
            }
        }
    }
}

unsigned char CProfileManager::ReadRawFileDataToAttributes(const unsigned char* data, unsigned int size,
                                                           CSaveRestoreInterface* collection)
{
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    unsigned int capacity = size + 1;
    CArrayInputStream_gServe stream;
    wchar_t* text = new wchar_t[capacity];
    memset(text, 0, capacity * sizeof(wchar_t));
    unsigned char loaded = 0;

    // Plain format: the JSON as JMUtf (JMUtf32 for big files).
    if (stream.Open(data, size)) {
        int length;
        if (size < 0x10002) {
            length = stream.ReadJMUtf(text, capacity);
        } else {
            length = stream.ReadJMUtf32(text, capacity);
        }
        text[length] = 0;
        loaded = 0;
        if (length > 0) {
            CHash attributes;
            attributes.Init(10, 10, 1);
            CStrWChar json(text);
            CObjectMapObject* root = (CObjectMapObject*)CNGSJSONParser::decodeValue(json);
            attributeManager->extractAttributeValuesAsHashMap(root, attributes);
            collection->setAllData(&attributes);
            if (root != NULL) {
                delete root;
            }
            DeleteAttributes(attributes);
            loaded = 1;
        }
        stream.Close();
        if (loaded) {
            if (text != NULL) {
                delete[] text;
            }
            return loaded;
        }
    }

    // Obfuscated format: CRC, text length, key length, key + text.
    loaded = 0;
    if (stream.Open(data, size)) {
        unsigned int crc = stream.ReadUInt32();
        unsigned int textLength = stream.ReadUInt32();
        unsigned int keyLength = stream.ReadUInt32();
        loaded = 0;
        unsigned int bytes = (keyLength + textLength) * 4;
        if (bytes <= size - 12) {
            stream.Read((unsigned char*)text, bytes);
            text[bytes + 1] = 0;
            sDataBlock block;
            if (CNGSUtil::DeObfuscateString(block, text, PROFILE_FILE_KEY, textLength, keyLength) == 0) {
                char* plain = (char*)malloc(block.m_size + (1 - keyLength));
                wchar_t* plainText = (wchar_t*)block.m_data + keyLength;
                wcstombs(plain, plainText, wcslen(plainText) + 1);
                loaded = 0;
                if (CCrc32::GetInstance()->Crc32((unsigned char*)plain, strlen(plain)) == crc) {
                    CStrWChar json(plain);
                    CObjectMapObject* root = (CObjectMapObject*)CNGSJSONParser::decodeValue(json);
                    CHash attributes;
                    attributes.Init(10, 10, 1);
                    attributeManager->extractAttributeValuesAsHashMap(root, attributes);
                    collection->setAllData(&attributes);
                    if (root != NULL) {
                        delete root;
                    }
                    DeleteAttributes(attributes);
                    loaded = 1;
                }
                free(block.m_data);
                free(plain);
            }
        }
    }
    stream.Close();
    if (text != NULL) {
        delete[] text;
    }
    return loaded;
}

void CProfileManager::useDataFromLocal(const CVector<int>& collectionTypes)
{
    copyDataBuffer(collectionTypes, &m_currentData, &m_localData);
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
        if (data->getFile() != NULL) {
            data->getFile()->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
        }
    }
}

unsigned char CProfileManager::invalidateLocalData(const CVector<int>& collectionTypes, unsigned char reset)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    unsigned char success = 1;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (localUser->m_credentials.m_userId != -1) {
            if (data->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED
                && loadFromFileSystem(collectionTypes[index], localUser->m_credentials.m_userId)) {
                data->copyAllData(*getGameData(collectionTypes[index], GAME_DATA_LOCAL));
            }
            data->setDataSaveStatus(PROFILE_DATA_INVALIDATED);
            if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                success = 0;
            }
        } else {
            success = 0;
        }
        if (reset && data != NULL) {
            data->resetAllData();
            if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                CSaveRestoreInterface* descriptor = data->getDescriptor();
                if (descriptor != NULL) {
                    descriptor->resetAllData();
                }
            }
        }
    }
    return success;
}

void CProfileManager::useDataFromServer(const CVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    copyDataBuffer(collectionTypes, &m_currentData, &m_serverData);
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        data->setDataSaveStatus(PROFILE_DATA_SYNCED);
        saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath());
        if (data->getFile() != NULL) {
            data->getFile()->setDataSaveStatus(PROFILE_DATA_SYNCED);
            saveToFileSystem(data->getFile(), localUser->m_credentials.getSubDirectoryPath());
        }
    }
}

bool CProfileManager::save(const CVector<int>& collectionTypes, bool syncToServer)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    bool saved = false;
    if (isAvailable()) {
        // Nothing is saved while an attribute collection was never loaded.
        for (int index = 0; index < collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() != BACKING_STORE_CONTENT_FILE && data->getDataSaveStatus() == PROFILE_DATA_NOT_LOADED) {
                return false;
            }
        }
        loadSaveStart();
        saved = true;
        for (int index = 0; index < collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
            switch (data->getDataSaveStatus()) {
            case PROFILE_DATA_NOT_LOADED:
                data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                // fall through
            case PROFILE_DATA_SAVED_LOCAL:
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            case PROFILE_DATA_SYNCED:
                data->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            case PROFILE_DATA_INVALIDATED:
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    saved = false;
                }
                break;
            default:
                saved = false;
                break;
            }
        }
        if (saved && syncToServer && localUser->isValid()) {
            m_pendingCollections.Clear();
            m_savingCollections = collectionTypes;
            syncWithServer(collectionTypes);
        } else {
            loadSaveEnd();
            if (saved) {
                localUser->m_notificationHandler->HandleProfileSaved(saved, collectionTypes);
            }
        }
    }
    return saved;
}

bool CProfileManager::uploadFileBackedData(const CVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    bool uploaded = false;
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        char fileName[128];
        {
            CStrWChar wideName = data->getFilename();
            wcstombs(fileName, wideName.GetWCharPtr(), wcslen(wideName.GetWCharPtr()) + 1);
        }
        // Upload to the copy that is not current: "<file>.A" / "<file>.B".
        CProfileDataDescriptor* descriptor = (CProfileDataDescriptor*)data->getDescriptor();
        if (descriptor != NULL) {
            descriptor->switchAB();
            char suffix[3];
            suffix[0] = '.';
            suffix[1] = (char)descriptor->getAB();
            suffix[2] = 0;
            descriptor->switchAB();
            strcat(fileName, suffix);
        }
        contentManager->Unlock();
        if (contentManager->getContentManagerStatus() == CONTENT_MANAGER_READY) {
            unsigned int size = CFileUtil_gServe::GetFilesize(
                CNGSUserCredentials::getSubDirectoryPath(localUser->m_credentials.m_userId).GetWCharPtr(),
                data->getFilename().GetWCharPtr());
            unsigned char* buffer = new unsigned char[size];
            memset(buffer, 0, size);
            uploaded = CFileUtil_gServe::ReadApplicationDataFile(
                           data->getFilename().GetWCharPtr(), buffer, size,
                           CNGSUserCredentials::getSubDirectoryPath(localUser->m_credentials.m_userId).GetWCharPtr()) != 0;
            if (uploaded) {
                contentManager->UploadProfileContent(fileName, buffer, size, data);
            }
            if (buffer != NULL) {
                delete[] buffer;
            }
        }
    }
    return uploaded;
}

bool CProfileManager::syncWithServer(const CVector<int>& collectionTypes)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
    CVector<CNGSAttribute*> attributes;
    CVector<int> collections(collectionTypes);
    if (!isLocalDataValid(collections)) {
        loadSaveEnd();
        localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
    } else {
        m_fileBackedCollections.Clear();
        bool uploading = false;
        bool hasContentFiles = false;
        for (int index = 0; index < collections.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(collections[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                m_fileBackedCollections.Add(collections[index]);
            } else if (data->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
                hasContentFiles = true;
                if (localUser->m_session->isSessionKeyValid() == 1) {
                    uploading = true;
                    CVector<int> single;
                    single.Add(collections[index]);
                    uploadFileBackedData(single);
                    m_pendingCollections.Add(collections[index]);
                }
            }
        }
        if (!uploading) {
            if (hasContentFiles) {
                loadSaveEnd();
                localUser->m_notificationHandler->HandleProfileSaved(0, collectionTypes);
            } else {
                for (int index = 0; index < collections.Size(); index++) {
                    CSaveRestoreInterface* data = getGameData(collections[index], GAME_DATA_CURRENT);
                    if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                        data->getAllData(attributes);
                        m_pendingCollections.Add(collections[index]);
                    }
                }
            }
        }
        if (attributes.Size() < 1) {
            return uploading;
        }
        AddWriteRequestOutstanding();
        attributeManager->saveDataToServer(
            attributes, new ProfileManagerFunctor(this, &CProfileManager::handleResponseSyncWithServer, collections),
            &localUser->m_credentials, 0);
    }
    return true;
}

unsigned char CProfileManager::isLocalDataValid(const CVector<int>& collectionTypes)
{
    for (int index = 0; index < collectionTypes.Size(); index++) {
        CSaveRestoreInterface* data = getGameData(collectionTypes[index], GAME_DATA_CURRENT);
        if (data != NULL && data->getBackingStoreType() != BACKING_STORE_CONTENT_FILE
            && data->getDataSaveStatus() == PROFILE_DATA_INVALIDATED) {
            return 0;
        }
    }
    return 1;
}

void CProfileManager::handleResponseSyncWithServer(CObjectMap* response, ProfileManagerFunctor* functor)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CVector<int> failed;
    if (CNGSServerObject::WasErrorInResponse(response, functor, "CProfileManager::handleResponseSyncWithServer") == 0) {
        for (int index = 0; index < functor->m_collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(functor->m_collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                data->setDataSaveStatus(PROFILE_DATA_SYNCED);
                if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                    failed.Add(functor->m_collectionTypes[index]);
                }
                m_pendingCollections.Remove(functor->m_collectionTypes[index]);
            }
        }
        CompleteWriteRequestOutstanding();
        if (m_pendingCollections.Size() != 0) {
            return;
        }
    } else {
        CompleteWriteRequestOutstanding();
    }
    loadSaveEnd();
    localUser->m_notificationHandler->HandleProfileSaved(0, m_savingCollections);
}

unsigned char CProfileManager::saveToFileSystem(CSaveRestoreInterface* collection, const CStrWChar& subDirectory)
{
    unsigned char saved = 0;
    unsigned char* data = NULL;
    unsigned int size = 0;
    if (WriteAttributesToRawFileData(collection, data, size) && data != NULL) {
        saved = 0;
        if (size != 0) {
            CStrWChar directory = CNGSUserCredentials::getFullDirectoryPathForSubdir(subDirectory);
            saved = 0;
            if (CNGSFileMgr::MkDir(directory.GetWCharPtr())) {
                saved = CFileUtil_gServe::WriteApplicationDataFile(collection->getFilename().GetWCharPtr(), data, size,
                                                                   subDirectory.GetWCharPtr());
            }
        }
        if (data != NULL) {
            delete[] data;
            data = NULL;
        }
    }
    // A content file collection also writes its attribute backed descriptor.
    if (collection->getBackingStoreType() == BACKING_STORE_CONTENT_FILE) {
        CSaveRestoreInterface* descriptor = collection->getDescriptor();
        if (descriptor != NULL && descriptor->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
            descriptor->setDataSaveStatus(collection->getDataSaveStatus());
            saveToFileSystem(descriptor, subDirectory);
        }
    }
    return saved;
}

unsigned char CProfileManager::WriteAttributesToRawFileData(CSaveRestoreInterface* collection, unsigned char*& data,
                                                            unsigned int& size)
{
    data = NULL;
    size = 0;
    CVector<CNGSAttribute*> attributes;
    collection->getAllData(attributes);
    CObjectMapValue* message = CAttributeManager::GetInstance()->createAttributeMessageObject(attributes);
    CStrWChar json = CNGSJSONParser::encodeValue(message);
    if (message != NULL) {
        delete message;
    }

    unsigned char written = 0;
    unsigned int fileSize = 0;
    bool sized = false;
    sDataBlock block;
    if (!m_obfuscateFiles) {
        unsigned int utfLength = CUtf::GetUtfLen(json.GetWCharPtr(), json.GetLength());
        if (utfLength != 0) {
            // JMUtf: 16-bit length prefix, JMUtf32 above 64 KB.
            fileSize = utfLength + 3;
            if (fileSize > 0xffff) {
                fileSize = utfLength + 5;
            }
            sized = true;
        }
    } else {
        if (CNGSUtil::ObfuscateString(block, json.GetWCharPtr(), PROFILE_FILE_KEY) == 0) {
            fileSize = block.m_size + 4;
            sized = true;
        }
    }
    if (sized && fileSize != 0) {
        unsigned char* buffer = new unsigned char[fileSize];
        memset(buffer, 0, fileSize);
        CArrayOutputStream_gServe stream;
        if (stream.Open(buffer, fileSize)) {
            if (!m_obfuscateFiles) {
                if (fileSize >= 0x10000) {
                    stream.WriteJMUtf32(json.GetWCharPtr());
                } else {
                    stream.WriteJMUtf(json.GetWCharPtr());
                }
            } else {
                // CRC of the plain JSON, then the obfuscated block.
                char plain[json.GetLength() + 1];
                wcstombs(plain, json.GetWCharPtr(), wcslen(json.GetWCharPtr()) + 1);
                unsigned int crc = CCrc32::GetInstance()->Crc32((unsigned char*)plain, json.GetLength());
                stream.WriteUInt32(crc);
                stream.Write((unsigned char*)block.m_data, block.m_size);
                free(block.m_data);
            }
            stream.Close();
            data = buffer;
            size = fileSize;
            return 1;
        }
        if (buffer != NULL) {
            delete[] buffer;
        }
        return 0;
    }
    return written;
}

CSaveRestoreInterface* CProfileManager::getManagedObjectFromBuffer(ProfileDataSaveStatus status, int collectionType)
{
    if (status == PROFILE_DATA_SAVED_LOCAL) {
        return getGameData(collectionType, GAME_DATA_LOCAL);
    }
    return getGameData(collectionType, GAME_DATA_SERVER);
}

unsigned char CProfileManager::HandleContentFileUploaded(unsigned char success, CSaveRestoreInterface* collection)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    if (collection != NULL) {
        CSaveRestoreInterface* descriptor = collection->getDescriptor();
        // The uploaded name without its ".A" / ".B" suffix.
        CStrWChar uploadedName(contentManager->m_fileName.GetWCharPtr());
        uploadedName = uploadedName.GetSubString(0, uploadedName.GetLength() - 3);
        bool sameFile = collection->getFilename() == uploadedName;
        if (success && sameFile) {
            collection->setDataSaveStatus(PROFILE_DATA_SYNCED);
            if (saveToFileSystem(collection, localUser->m_credentials.getSubDirectoryPath()) && descriptor != NULL) {
                CVector<int> collectionTypes;
                int descriptorType = descriptor->getCollectionType();
                collectionTypes.Add(descriptorType);
                ((CProfileDataDescriptor*)descriptor)->switchAB();
                if (isLocalDataValid(collectionTypes)) {
                    CVector<CNGSAttribute*> attributes;
                    CAttributeManager* attributeManager = CAttributeManager::GetInstance();
                    for (int index = 0; index < m_fileBackedCollections.Size(); index++) {
                        CSaveRestoreInterface* data = getGameData(m_fileBackedCollections[index], GAME_DATA_CURRENT);
                        if (data->getBackingStoreType() == BACKING_STORE_ATTRIBUTES) {
                            data->getAllData(attributes);
                            m_pendingCollections.Add(m_fileBackedCollections[index]);
                            collectionTypes.Add(m_fileBackedCollections[index]);
                        }
                    }
                    m_fileBackedCollections.Clear();
                    descriptor->getAllData(attributes);
                    int pendingType = descriptor->getCollectionType();
                    m_pendingCollections.Add(pendingType);
                    if (attributes.Size() > 0) {
                        AddWriteRequestOutstanding();
                        attributeManager->saveDataToServer(
                            attributes,
                            new ProfileManagerFunctor(this, &CProfileManager::handleResponseSyncDataDescWithServer, collectionTypes),
                            &localUser->m_credentials, 0);
                    }
                }
                ((CProfileDataDescriptor*)descriptor)->switchAB();
            }
        }
        int collectionType = collection->getCollectionType();
        m_pendingCollections.Remove(collectionType);
        if (m_pendingCollections.Size() == 0) {
            loadSaveEnd();
            localUser->m_notificationHandler->HandleProfileSaved(success, m_savingCollections);
        }
    }
    return 1;
}

void CProfileManager::handleResponseSyncDataDescWithServer(CObjectMap* response, ProfileManagerFunctor* functor)
{
    unsigned char success = 0;
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CVector<int> failed;
    if (CNGSServerObject::WasErrorInResponse(response, functor, "CProfileManager::handleResponseSyncDataDescWithServer") == 0) {
        success = 1;
        for (int index = 0; index < functor->m_collectionTypes.Size(); index++) {
            CSaveRestoreInterface* data = getGameData(functor->m_collectionTypes[index], GAME_DATA_CURRENT);
            if (data->getFile() != NULL) {
                ((CProfileDataDescriptor*)data)->switchAB();
            }
            data->setDataSaveStatus(PROFILE_DATA_SYNCED);
            if (!saveToFileSystem(data, localUser->m_credentials.getSubDirectoryPath())) {
                failed.Add(functor->m_collectionTypes[index]);
            }
        }
    }
    m_pendingCollections.RemoveAll(functor->m_collectionTypes);
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        localUser->m_notificationHandler->HandleProfileSaved(success, m_savingCollections);
    }
    CompleteWriteRequestOutstanding();
}

unsigned char CProfileManager::HandleContentFileDownloadedSelf(unsigned char success, const unsigned char* data, int size,
                                                               CSaveRestoreInterface* collection)
{
    CNGSLocalUser* localUser = CNGS::GetInstance()->GetLocalUser();
    CNGSContentManager* contentManager = CNGSContentManager::GetInstance();
    if (collection == NULL) {
        return 1;
    }
    CSaveRestoreInterface* serverData = getGameData(collection->getCollectionType(), GAME_DATA_SERVER);
    CSaveRestoreInterface* currentData = getGameData(collection->getCollectionType(), GAME_DATA_CURRENT);
    // The downloaded name without its ".A" / ".B" suffix.
    CStrWChar downloadedName(contentManager->m_fileName.GetWCharPtr());
    downloadedName = downloadedName.GetSubString(0, downloadedName.GetLength() - 3);
    if (downloadedName == collection->getFilename()) {
        if (success) {
            if (collection->isDataFormatValid(data, size)) {
                serverData->setDataSaveStatus(PROFILE_DATA_SYNCED);
                if (ReadRawFileDataToAttributes(data, size, serverData)) {
                    if (currentData == collection) {
                        currentData->copyAllData(*serverData);
                        currentData->setDataSaveStatus(PROFILE_DATA_SYNCED);
                        saveToFileSystem(currentData, localUser->m_credentials.getSubDirectoryPath());
                    }
                } else {
                    serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
                    success = 0;
                }
            } else {
                success = 0;
                CSaveRestoreInterface* localData = getGameData(collection->getCollectionType(), GAME_DATA_LOCAL);
                if (localData->getDataSaveStatus() == PROFILE_DATA_INVALIDATED && serverData == collection) {
                    // The server copy is unusable: push the local data instead.
                    CVector<int> collectionTypes;
                    getAllRegisteredCollections(collectionTypes);
                    useDataFromLocal(collectionTypes);
                    save(collectionTypes, false);
                }
            }
        } else {
            serverData->setDataSaveStatus(PROFILE_DATA_SAVED_LOCAL);
            success = 0;
        }
    }
    int collectionType = collection->getCollectionType();
    m_pendingCollections.Remove(collectionType);
    if (m_pendingCollections.Size() == 0) {
        loadSaveEnd();
        if (currentData == collection || !success) {
            localUser->m_notificationHandler->HandleProfileLoaded(success, m_savingCollections);
        } else if (serverData == collection) {
            localUser->m_notificationHandler->HandleProfileDataConflict(m_savingCollections);
        }
    }
    return 1;
}

unsigned char CProfileManager::isAvailable()
{
    if (!isReady() || m_writeRequestsOutstanding != 0) {
        return 0;
    }
    if (m_registeredData.Size() < 1) {
        return 1;
    }
    return CNGSContentManager::GetInstance()->getContentManagerStatus() == CONTENT_MANAGER_READY;
}
