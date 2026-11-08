// CBigFile_v2.cpp - version 2 resource archive.
// Layout: header, hash table (bucket -> info block offset), info block (4 words per resource),
// mime key table, optional name compression table, string block. Resource data stays in the file
// and is read through one of the stream slots.
#include "CBigFile_v2.h"
#include "CResourceDataBlockFileAccess.h"
#include "CStringToKey.h"
#include "ICFile.h"
#include "ICFileMgr.h"

namespace com { namespace glu { namespace platform { namespace components {

/** Escape byte of a compressed name: followed by a 1 or 2 byte fragment index (1-based). */
static const char kNameCompressionEscape = 0x1a;

unsigned char CBigFile_v2_Handler_OnClose(void* context, CInputStream& stream)
{
    CBigFile_v2::CBigFileStreamManager* manager = (CBigFile_v2::CBigFileStreamManager*)context;
    for (unsigned int i = 0; i < manager->m_count; i++) {
        if (manager->m_slots[i].m_owner == &stream) {
            return manager->Release(&manager->m_slots[i]);
        }
    }
    return 0;
}

unsigned char CBigFile_v2::CStringBlockItr::Initialize(CBigFile_v2& bigFile, unsigned int nameKey, unsigned int count)
{
    m_bigFile = &bigFile;
    m_index = 0;
    m_count = count;
    unsigned char wasDecompressed;
    m_current = bigFile.GetName(nameKey, 0, wasDecompressed);
    return m_current != NULL;
}

const char* CBigFile_v2::GetName(unsigned int nameKey, unsigned char decompress, unsigned char& wasDecompressed)
{
    const char* result = NULL;
    wasDecompressed = 0;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry != NULL) {
        m_fetcher.Initialize(entry, m_mimeKeys);
        unsigned int nameOffset;
        unsigned int mimeKey;
        unsigned int size;
        unsigned int dataOffset;
        unsigned int flags;
        if (m_fetcher.Fetch(nameOffset, mimeKey, size, dataOffset, flags)) {
            const char* storedName = m_strings + nameOffset;
            if (m_compressionTable != NULL) {
                unsigned char decompressed = DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, storedName);
                wasDecompressed = decompressed;
                const char* plainName = m_nameBuffer;
                if (!decompressed) {
                    plainName = storedName;
                }
                if ((core::CStringToKey(plainName, 0) & 0x7fffffff) == nameKey) {
                    result = storedName;
                    if (decompress) {
                        result = plainName;
                    }
                }
            } else if ((core::CStringToKey(storedName, 0) & 0x7fffffff) == nameKey) {
                result = storedName;
            }
        }
    }
    return result;
}

unsigned char CBigFile_v2::CStringBlockItr::Initialize(CBigFile_v2& bigFile, const char* name, unsigned int count)
{
    m_bigFile = &bigFile;
    m_index = 0;
    m_count = count;
    unsigned char wasDecompressed;
    m_current = bigFile.GetName(name, 0, wasDecompressed);
    return m_current != NULL;
}

const char* CBigFile_v2::GetName(const char* name, unsigned char decompress, unsigned char& wasDecompressed)
{
    unsigned int nameKey = core::CStringToKey(name, 0) & 0x7fffffff;
    wasDecompressed = 0;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry == NULL) {
        return NULL;
    }
    m_fetcher.Initialize(entry, m_mimeKeys);
    unsigned int nameOffset;
    unsigned int mimeKey;
    unsigned int size;
    unsigned int dataOffset;
    unsigned int flags;
    const char* storedName;
    do {
        if (!m_fetcher.Fetch(nameOffset, mimeKey, size, dataOffset, flags)) {
            return NULL;
        }
        storedName = m_strings + nameOffset;
    } while (!AreStringsEqual_FirstStringIsGuaranteedUncompressed(name, storedName));
    if (!decompress) {
        return storedName;
    }
    unsigned char decompressed = DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, storedName);
    wasDecompressed = decompressed;
    if (decompressed) {
        return m_nameBuffer;
    }
    return storedName;
}

/** Move to the next name of the string block. */
void CBigFile_v2::CStringBlockItr::operator++(int)
{
    if (m_current == NULL || m_index >= m_count) {
        return;
    }
    // skip the current name, then the padding up to the next one
    while (*m_current != 0) {
        m_current++;
    }
    while (*m_current == 0) {
        m_current++;
    }
    m_index++;
}

/** Move to the previous name of the string block. */
void CBigFile_v2::CStringBlockItr::operator--(int)
{
    if (m_current == NULL || m_index == 0) {
        return;
    }
    do {
        m_current--;
    } while (*m_current == 0);
    do {
        m_current--;
    } while (*m_current != 0);
    m_current++;
    m_index--;
}

unsigned char CBigFile_v2::InfoBlockFetcher::Fetch(unsigned int& nameOffset, unsigned int& mimeKey, unsigned int& size,
                                                   unsigned int& dataOffset, unsigned int& flags)
{
    if (m_remaining == 0) {
        return 0;
    }
    nameOffset = *m_cursor;
    m_cursor++;
    dataOffset = *m_cursor;
    m_cursor++;
    size = *m_cursor;
    m_cursor++;
    // high half: mime key index, low half: flags
    unsigned int packed = *m_cursor;
    unsigned int key = 0;
    if (m_mimeKeys != NULL) {
        key = m_mimeKeys[packed >> 16];
    }
    mimeKey = key;
    flags = packed & 0xffff;
    m_cursor++;
    m_remaining--;
    return 1;
}

CBigFile_v2::~CBigFile_v2()
{
    Destroy();
}

void CBigFile_v2::Destroy()
{
    for (unsigned int i = 0; i < m_streamManager.m_count; i++) {
        if (m_streamManager.m_slots[i].m_file != NULL) {
            delete m_streamManager.m_slots[i].m_file;
        }
        m_streamManager.m_slots[i].m_file = NULL;
    }
    delete[] m_streamManager.m_slots;
    m_streamManager.m_slots = NULL;
    m_streamManager.m_count = 0;
    if (m_strings != NULL) {
        free(m_strings);
        m_strings = NULL;
    }
    if (m_compressionTable != NULL) {
        free(m_compressionTable);
        m_compressionTable = NULL;
    }
    if (m_mimeKeys != NULL) {
        free(m_mimeKeys);
        m_mimeKeys = NULL;
    }
    if (m_infoBlock != NULL) {
        free(m_infoBlock);
        m_infoBlock = NULL;
    }
    if (m_hashTable != NULL) {
        free(m_hashTable);
        m_hashTable = NULL;
    }
    ClearMembers();
}

void CBigFile_v2::ClearMembers()
{
    m_hashType = 0;
    m_hashTableOffset = 0;
    m_hashTableSize = 0;
    m_infoBlockOffset = 0;
    m_infoBlockSize = 0;
    m_mimeKeysOffset = 0;
    m_mimeKeysSize = 0;
    m_compressionTableOffset = 0;
    m_compressionTableSize = 0;
    m_stringsOffset = 0;
    m_stringsSize = 0;
    m_dataOffset = 0;
    m_dataSize = 0;
    m_hashBucketCount = 0;
    m_hashTable = NULL;
    m_infoBlock = NULL;
    m_mimeKeys = NULL;
    m_compressionTokenBytes = 0;
    m_compressionTable = NULL;
    m_strings = NULL;
}

/** Allocate `size` bytes and fill them from `stream`. */
static inline unsigned char* ReadBlock(CInputStream& stream, unsigned int size)
{
    unsigned char* block = (unsigned char*)malloc(size);
    stream.Read(block, size);
    return block;
}

unsigned char CBigFile_v2::Load(CInputStream& stream, unsigned int size)
{
    m_signature[4] = 0;
    stream.Read((unsigned char*)m_signature, 4);
    m_versionMajor = stream.ReadUInt8();
    m_versionMinor = stream.ReadUInt8();
    if (m_versionMajor < 2) {
        return 0;
    }
    m_versionRevision = stream.ReadUInt8();
    m_flags = stream.ReadUInt8();

    char hashName[5];
    hashName[4] = 0;
    stream.Read((unsigned char*)hashName, 4);
    if (strcmp(hashName, "djb2") == 0 || strcmp(hashName, "s2k1") == 0 || strcmp(hashName, "sdbm") == 0) {
        m_hashType = 2;
    } else if (strcmp(hashName, "crxx") == 0) {
        m_hashType = 4;
    } else {
        Destroy();
        return 0;
    }

    m_hashTableOffset = stream.ReadUInt32();
    m_hashTableSize = stream.ReadUInt32();
    m_infoBlockOffset = stream.ReadUInt32();
    m_infoBlockSize = stream.ReadUInt32();
    m_mimeKeysOffset = stream.ReadUInt32();
    m_mimeKeysSize = stream.ReadUInt32();
    m_compressionTableOffset = stream.ReadUInt32();
    m_compressionTableSize = stream.ReadUInt32();
    m_stringsOffset = stream.ReadUInt32();
    m_stringsSize = stream.ReadUInt32();
    m_dataOffset = stream.ReadUInt32();
    m_dataSize = stream.ReadUInt32();

    m_hashTable = (unsigned int*)ReadBlock(stream, m_hashTableSize);
    m_infoBlock = ReadBlock(stream, m_infoBlockSize);
    m_mimeKeys = (unsigned int*)ReadBlock(stream, m_mimeKeysSize);
    if (m_compressionTableSize != 0) {
        m_compressionTable = (unsigned int*)ReadBlock(stream, m_compressionTableSize);
    } else {
        m_compressionTable = NULL;
    }
    m_strings = (char*)ReadBlock(stream, m_stringsSize);
    m_hashBucketCount = m_hashTableSize / sizeof(unsigned int);

    if (m_compressionTableSize != 0) {
        if (m_compressionTableSize <= 0xff) {
            m_compressionTokenBytes = 1;
        } else if (m_compressionTableSize < 0x10000) {
            m_compressionTokenBytes = 2;
        } else {
            Destroy();
            return 0;
        }
    }

    // every stream slot gets its own handle on the archive file
    ICFileMgr* fileMgr = ICFileMgr::GetInstance();
    const wchar_t* path = CResourceDataBlockFileAccess::GetInstance()->m_path.m_string;
    m_streamManager.Initialize(4);
    for (unsigned int i = 0; i < m_streamManager.m_count; i++) {
        m_streamManager.m_slots[i].m_file = fileMgr->Open(path, ICFileMgr::AccessRead);
    }
    return 1;
}

unsigned char CBigFile_v2::Load(const wchar_t* path)
{
    CResourceDataBlockFileAccess::GetInstance()->m_path = path;
    ICFileMgr* fileMgr = ICFileMgr::GetInstance();
    unsigned char result = 0;
    ICFile* file = fileMgr->Open(path, ICFileMgr::AccessRead);
    if (file != NULL) {
        CFileInputStream stream;
        if (stream.Open(file)) {
            result = Load(stream, 0);
        }
        fileMgr->Close(file);
    }
    return result;
}

unsigned int CBigFile_v2::GetMimeKey(unsigned int nameKey)
{
    unsigned int result = 0;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry != NULL) {
        m_fetcher.Initialize(entry, m_mimeKeys);
        unsigned int nameOffset;
        unsigned int mimeKey;
        unsigned int size;
        unsigned int dataOffset;
        unsigned int flags;
        if (m_fetcher.Fetch(nameOffset, mimeKey, size, dataOffset, flags)) {
            const char* storedName = m_strings + nameOffset;
            const char* plainName;
            if (m_compressionTable != NULL) {
                plainName = m_nameBuffer;
                if (!DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, storedName)) {
                    plainName = storedName;
                }
            } else {
                plainName = storedName;
            }
            if ((core::CStringToKey(plainName, 0) & 0x7fffffff) == nameKey) {
                result = mimeKey;
            }
        }
    }
    return result;
}

unsigned char CBigFile_v2::DecompressIntoDestinationBufferIfNeeded(char* destination, const char* source) const
{
    if (m_compressionTable == NULL) {
        return 0;
    }
    // only names containing an escape byte are compressed
    const char* scan = source;
    while (*scan != kNameCompressionEscape) {
        if (*scan == 0) {
            return 0;
        }
        scan++;
    }
    const char* fragment = NULL;
    for (;;) {
        if (fragment != NULL) {
            // copy the current fragment
            char fragmentChar = *fragment;
            if (fragmentChar != 0) {
                *destination++ = fragmentChar;
                fragment++;
            } else {
                fragment = NULL;
            }
            continue;
        }
        char c = *source;
        if (c == 0) {
            break;
        }
        if (c == kNameCompressionEscape) {
            int index = source[1];
            if (m_compressionTokenBytes == 1) {
                source += 2;
            } else {
                index = index | (source[2] << 8);
                source += 3;
            }
            fragment = m_strings + m_compressionTable[index - 1];
        } else {
            *destination++ = c;
            source++;
        }
    }
    *destination = 0;
    return 1;
}

unsigned int CBigFile_v2::GetMimeKey(const char* name)
{
    unsigned int nameKey = core::CStringToKey(name, 0) & 0x7fffffff;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry == NULL) {
        return 0;
    }
    m_fetcher.Initialize(entry, m_mimeKeys);
    unsigned int nameOffset;
    unsigned int mimeKey;
    unsigned int size;
    unsigned int dataOffset;
    unsigned int flags;
    do {
        if (!m_fetcher.Fetch(nameOffset, mimeKey, size, dataOffset, flags)) {
            return 0;
        }
    } while (!AreStringsEqual_FirstStringIsGuaranteedUncompressed(name, m_strings + nameOffset));
    return mimeKey;
}

unsigned char CBigFile_v2::AreStringsEqual_FirstStringIsGuaranteedUncompressed(const char* plain, const char* stored) const
{
    if (m_compressionTable == NULL) {
        return strcmp(plain, stored) == 0;
    }
    // walk `stored` expanding fragments on the fly
    const char* fragment = NULL;
    for (;;) {
        char plainChar = *plain;
        unsigned char plainEnded = (plainChar == 0);
        if (!plainEnded) {
            plain++;
        }
        char storedChar;
        for (;;) {
            if (fragment != NULL) {
                storedChar = *fragment;
                if (storedChar != 0) {
                    fragment++;
                    break;
                }
                fragment = NULL;
                continue;
            }
            storedChar = *stored;
            if (storedChar == 0) {
                break;
            }
            if (storedChar != kNameCompressionEscape) {
                stored++;
                break;
            }
            int index = stored[1];
            if (m_compressionTokenBytes == 1) {
                stored += 2;
            } else {
                index = index | (stored[2] << 8);
                stored += 3;
            }
            fragment = m_strings + m_compressionTable[index - 1];
        }
        if (plainChar != storedChar) {
            return 0;
        }
        if (plainEnded) {
            return 1;
        }
    }
}

unsigned char CBigFile_v2::GetStream(unsigned int nameKey, CInputStream& stream, unsigned int& mimeKey,
                                     const char** outName, unsigned char decompressName, unsigned char& wasDecompressed)
{
    wasDecompressed = 0;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry == NULL) {
        return 0;
    }
    m_fetcher.Initialize(entry, m_mimeKeys);
    unsigned int nameOffset;
    unsigned int entryMimeKey;
    unsigned int size;
    unsigned int dataOffset;
    unsigned int flags;
    if (!m_fetcher.Fetch(nameOffset, entryMimeKey, size, dataOffset, flags)) {
        return 0;
    }
    const char* name = m_strings + nameOffset;
    if (m_compressionTable != NULL) {
        unsigned char decompressed = DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, name);
        wasDecompressed = decompressed;
        const char* plainName = name;
        if (decompressed) {
            plainName = m_nameBuffer;
        }
        if ((core::CStringToKey(plainName, 0) & 0x7fffffff) != nameKey) {
            return 0;
        }
        if (!GetResourceDataStream(stream, dataOffset, size, flags)) {
            return 0;
        }
        mimeKey = entryMimeKey;
        if (outName == NULL) {
            return 1;
        }
        if (decompressName) {
            name = plainName;
        }
    } else {
        if ((core::CStringToKey(name, 0) & 0x7fffffff) != nameKey) {
            return 0;
        }
        if (!GetResourceDataStream(stream, dataOffset, size, flags)) {
            return 0;
        }
        mimeKey = entryMimeKey;
        if (outName == NULL) {
            return 1;
        }
    }
    *outName = name;
    return 1;
}

unsigned char CBigFile_v2::GetResourceDataStream(CInputStream& stream, unsigned int dataOffset, unsigned int& size,
                                                 unsigned int& flags)
{
    StreamSlot* slot = m_streamManager.Acquire(&stream);
    unsigned char result = 0;
    if (slot->m_file->Seek(dataOffset, ICFile::SeekSet)) {
        CFileInputStream* fileStream = &slot->m_fileStream;
        if (fileStream->Open(slot->m_file)) {
            CInputStream* source = fileStream;
            if (flags & 1) {
                // compressed: a uint32 compressed size precedes the zlib data
                source = &slot->m_zipStream;
                unsigned int compressedSize = fileStream->ReadUInt32();
                if (!slot->m_zipStream.Open(*fileStream, compressedSize, size)) {
                    source = NULL;
                }
            }
            if (source != NULL) {
                result = stream.Open(source, size, &m_streamManager, CBigFile_v2_Handler_OnClose);
            }
        }
    }
    if (slot != NULL && !result) {
        m_streamManager.Release(slot);
    }
    return result;
}

unsigned char CBigFile_v2::GetStream(const char* name, CInputStream& stream, unsigned int& mimeKey,
                                     const char** outName, unsigned char decompressName, unsigned char& wasDecompressed)
{
    unsigned int nameKey = core::CStringToKey(name, 0) & 0x7fffffff;
    const unsigned int* entry = GetInfoBlockEntry(nameKey);
    if (entry == NULL) {
        return 0;
    }
    m_fetcher.Initialize(entry, m_mimeKeys);
    unsigned int nameOffset;
    unsigned int entryMimeKey;
    unsigned int size;
    unsigned int dataOffset;
    unsigned int flags;
    do {
        if (!m_fetcher.Fetch(nameOffset, entryMimeKey, size, dataOffset, flags)) {
            return 0;
        }
    } while (!AreStringsEqual_FirstStringIsGuaranteedUncompressed(name, m_strings + nameOffset));
    if (!GetResourceDataStream(stream, dataOffset, size, flags)) {
        return 0;
    }
    mimeKey = entryMimeKey;
    if (outName == NULL) {
        return 1;
    }
    const char* storedName = m_strings + nameOffset;
    if (decompressName) {
        unsigned char decompressed = DecompressIntoDestinationBufferIfNeeded(m_nameBuffer, storedName);
        wasDecompressed = decompressed;
        if (decompressed) {
            *outName = m_nameBuffer;
        } else {
            *outName = storedName;
        }
    } else {
        *outName = storedName;
    }
    return 1;
}

} } } }
