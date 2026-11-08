// CBigFile_v2.h - version 2 resource archive: hashed directory, optional name compression,
// resources read through a small pool of file streams.
#ifndef CBIGFILE_V2_H
#define CBIGFILE_V2_H

#include "CBigFile.h"
#include "CFileInputStream.h"
#include "CZipInputStream.h"

namespace com { namespace glu { namespace platform { namespace components {

class ICFile;

/** CInputStream close handler: gives the stream slot of the closed resource stream back. */
unsigned char CBigFile_v2_Handler_OnClose(void* context, CInputStream& stream);

class CBigFile_v2 : public CBigFile {
public:
    /** Walks the info block entries of one hash bucket (4 words per entry). */
    class InfoBlockFetcher {
    public:
        inline void Initialize(const unsigned int* entry, const unsigned int* mimeKeys);
        /** Next entry: name offset, mime key, size, data offset and flags; false when done. */
        unsigned char Fetch(unsigned int& nameOffset, unsigned int& mimeKey, unsigned int& size,
                            unsigned int& dataOffset, unsigned int& flags);

        const unsigned int* m_cursor;       // 0x00
        unsigned int m_remaining;           // 0x04 entries left
        const unsigned int* m_mimeKeys;     // 0x08 mime key table (indexed by flags >> 16)
    };

    /** Iterates consecutive names of the string block, starting at a given resource. */
    class CStringBlockItr {
    public:
        unsigned char Initialize(CBigFile_v2& bigFile, unsigned int nameKey, unsigned int count);
        unsigned char Initialize(CBigFile_v2& bigFile, const char* name, unsigned int count);
        void operator++(int);
        void operator--(int);

        CBigFile_v2* m_bigFile;             // 0x00
        unsigned int m_index;               // 0x04
        unsigned int m_count;               // 0x08
        const char* m_current;              // 0x0c
    };

    /** One reusable set of streams to read a resource out of the archive file. */
    struct StreamSlot {
        unsigned char m_inUse;              // 0x000
        CInputStream* m_owner;              // 0x004 resource stream using this slot
        ICFile* m_file;                     // 0x008 own handle on the archive file
        CFileInputStream m_fileStream;      // 0x00c
        CZipInputStream m_zipStream;        // 0x034 for compressed resources
    };

    /** The pool of stream slots. */
    class CBigFileStreamManager {
    public:
        inline ~CBigFileStreamManager();
        inline void Initialize(unsigned int count);
        /** First free slot, now owned by `owner`; NULL when all are busy. */
        inline StreamSlot* Acquire(CInputStream* owner);
        /** Give `slot` back; false when it is not one of the pool. */
        inline unsigned char Release(StreamSlot* slot);

        StreamSlot* m_slots;                // 0x00
        unsigned int m_count;               // 0x04
    };

    /** Empty archive (no stream slots yet). */  // added by b011x
    inline CBigFile_v2()  // added by b011x
    {  // added by b011x
        m_fetcher.m_cursor = NULL;  // added by b011x
        m_fetcher.m_remaining = 0;  // added by b011x
        m_fetcher.m_mimeKeys = NULL;  // added by b011x
        m_streamManager.m_slots = NULL;  // added by b011x
        m_streamManager.m_count = 0;  // added by b011x
        ClearMembers();  // added by b011x
    }  // added by b011x

    virtual ~CBigFile_v2();

    virtual void Destroy();
    void ClearMembers();
    virtual unsigned char Load(CInputStream& stream, unsigned int size);
    virtual unsigned char Load(const wchar_t* path);
    virtual unsigned int GetMimeKey(unsigned int nameKey);
    virtual unsigned int GetMimeKey(const char* name);

    /**
     * Stored name of a resource (NULL when unknown). With `decompress` a compressed name is
     * expanded into m_nameBuffer; `wasDecompressed` tells whether that happened.
     */
    const char* GetName(unsigned int nameKey, unsigned char decompress, unsigned char& wasDecompressed);
    const char* GetName(const char* name, unsigned char decompress, unsigned char& wasDecompressed);

    /** Expand a compressed name into `destination`; false when `source` is not compressed. */
    unsigned char DecompressIntoDestinationBufferIfNeeded(char* destination, const char* source) const;
    /** Compare a plain name with a (possibly compressed) stored name. */
    unsigned char AreStringsEqual_FirstStringIsGuaranteedUncompressed(const char* plain, const char* stored) const;

    /** Open `stream` on a resource; optionally returns its mime key and stored name. */
    unsigned char GetStream(unsigned int nameKey, CInputStream& stream, unsigned int& mimeKey,
                            const char** outName, unsigned char decompressName, unsigned char& wasDecompressed);
    unsigned char GetStream(const char* name, CInputStream& stream, unsigned int& mimeKey,
                            const char** outName, unsigned char decompressName, unsigned char& wasDecompressed);
    unsigned char GetResourceDataStream(CInputStream& stream, unsigned int dataOffset, unsigned int& size,
                                        unsigned int& flags);

    /** Info block entry list of the hash bucket of `nameKey`, NULL when the bucket is empty. */
    inline const unsigned int* GetInfoBlockEntry(unsigned int nameKey) const;

    unsigned char m_flags;                  // 0x10
    int m_hashType;                         // 0x14 2 = djb2/s2k1/sdbm, 4 = crxx
    unsigned int m_hashTableOffset;         // 0x18 header: offset/size pairs of the blocks
    unsigned int m_hashTableSize;           // 0x1c
    unsigned int m_infoBlockOffset;         // 0x20
    unsigned int m_infoBlockSize;           // 0x24
    unsigned int m_mimeKeysOffset;          // 0x28
    unsigned int m_mimeKeysSize;            // 0x2c
    unsigned int m_compressionTableOffset;  // 0x30
    unsigned int m_compressionTableSize;    // 0x34
    unsigned int m_stringsOffset;           // 0x38
    unsigned int m_stringsSize;             // 0x3c
    unsigned int m_dataOffset;              // 0x40
    unsigned int m_dataSize;                // 0x44
    unsigned int m_hashBucketCount;         // 0x48
    unsigned int* m_hashTable;              // 0x4c info block offset per bucket, 0xffffffff = empty
    unsigned char* m_infoBlock;             // 0x50
    unsigned int* m_mimeKeys;               // 0x54
    int m_compressionTokenBytes;            // 0x58 1 or 2 bytes per name compression token
    unsigned int* m_compressionTable;       // 0x5c string offsets of the name fragments, NULL = no compression
    char* m_strings;                        // 0x60 string block
    InfoBlockFetcher m_fetcher;             // 0x64
    CBigFileStreamManager m_streamManager;  // 0x70
    char m_nameBuffer[0x100];               // 0x78 decompressed name
};

inline void CBigFile_v2::InfoBlockFetcher::Initialize(const unsigned int* entry, const unsigned int* mimeKeys)
{
    m_mimeKeys = mimeKeys;
    m_cursor = entry;
    unsigned int header = *entry;
    if (header & 0x80000000) {
        // a count word precedes a list of entries
        m_cursor = entry + 1;
        m_remaining = header & 0x7fffffff;
    } else {
        m_remaining = 1;
    }
}

inline CBigFile_v2::CBigFileStreamManager::~CBigFileStreamManager()
{
    delete m_slots;
}

inline void CBigFile_v2::CBigFileStreamManager::Initialize(unsigned int count)
{
    delete[] m_slots;
    m_slots = NULL;
    m_count = 0;
    m_slots = new StreamSlot[count];
    for (unsigned int i = 0; i < count; i++) {
        m_slots[i].m_inUse = 0;
        m_slots[i].m_owner = NULL;
        m_slots[i].m_file = NULL;
    }
    m_count = count;
}

inline CBigFile_v2::StreamSlot* CBigFile_v2::CBigFileStreamManager::Acquire(CInputStream* owner)
{
    for (unsigned int i = 0; i < m_count; i++) {
        if (!m_slots[i].m_inUse) {
            m_slots[i].m_inUse = 1;
            m_slots[i].m_owner = owner;
            return &m_slots[i];
        }
    }
    return NULL;
}

inline unsigned char CBigFile_v2::CBigFileStreamManager::Release(StreamSlot* slot)
{
    for (unsigned int i = 0; i < m_count; i++) {
        if (&m_slots[i] == slot) {
            m_slots[i].m_inUse = 0;
            m_slots[i].m_owner = NULL;
            return 1;
        }
    }
    return 0;
}

inline const unsigned int* CBigFile_v2::GetInfoBlockEntry(unsigned int nameKey) const
{
    unsigned int offset = m_hashTable[nameKey % m_hashBucketCount];
    if (offset == 0xffffffff) {
        return NULL;
    }
    return (const unsigned int*)(m_infoBlock + offset);
}

} } } }

#endif
