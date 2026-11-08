// CTypedVariableTable.h - a packed block of typed variables (shader parameter tables).
// Variables are declared with Add() and become part of the table on the next
// SyncNumberOfEntriesAndTableSize(), which rebuilds the packed storage, keeps the data of the
// entries that survived and notifies the registered sync handlers. (created by b006)
#ifndef CTYPEDVARIABLETABLE_H
#define CTYPEDVARIABLETABLE_H

#include <stdlib.h>
#include <wchar.h>  // added by b037x
#include "CClass.h"
#include "CStrWChar.h"
#include "CVector.h"

namespace com { namespace glu { namespace platform { namespace components {

class CTypedVariableTable : public core::CClass {
public:
    /** Runtime class id: CStringToKey("CTypedVariableTable"). */
    static const unsigned int ClassId = 0x2913666c;

    /** Variable kinds. Byte 3: kind index, byte 2: matrix flag, byte 1: base type, byte 0: components. */
    struct Entry {
        enum Type {
            Undefined   = 0,
            Boolean     = 0x01000101,
            Boolean2d   = 0x02000102,
            Boolean3d   = 0x03000103,
            Boolean4d   = 0x04000104,
            Float       = 0x05000201,
            Integer     = 0x06000301,
            Integer2d   = 0x07000302,
            Integer3d   = 0x08000303,
            Integer4d   = 0x09000304,
            Matrix2d    = 0x0a010204,
            Matrix3d    = 0x0b010209,
            Matrix4d    = 0x0c010210,
            Matrix2dx   = 0x0d010304,
            Matrix3dx   = 0x0e010309,
            Matrix4dx   = 0x0f010310,
            Sampler2d   = 0x10000301,
            SamplerCube = 0x11000301,
            Vector2d    = 0x12000202,
            Vector3d    = 0x13000203,
            Vector4d    = 0x14000204
        };

        /** Type for a type name ("float", "matrix4d", ..., case-insensitive); Undefined if unknown. */
        static Type GetType(const wchar_t* name);
    };

    /** Declaration of one variable. */
    struct EntryDesc {
        CStrWChar m_name;           // 0x00
        Entry::Type m_type;         // 0x10
        unsigned int m_count;       // 0x14 elements of m_type
        unsigned int m_arraySize;   // 0x18 history slots requested (see EntryData)
    };

    /** Called after every table rebuild. */
    struct SyncHandler {
        /** The callback returns a flag the table ignores (the shader program callback returns 1). (fx1) */
        typedef unsigned char (*Callback)(void* context, CTypedVariableTable* table);

        void* m_context;                                                // 0x00
        Callback m_callback;                                            // 0x04

        // a user-declared constructor matters: without one clang zero-fills the slot before
        // copy-constructing it in CVector::AddAllNoResize, which the original does not do
        SyncHandler() {}

        SyncHandler(void* context, Callback callback)
        {
            m_context = context;
            m_callback = callback;
        }
    };

    /** Per-entry runtime information, rebuilt by SyncNumberOfEntriesAndTableSize. */
    struct EntryData {
        Entry::Type m_type;         // 0x00
        unsigned int m_count;       // 0x04
        unsigned int m_arraySize;   // 0x08 extra slots in m_array (EntryDesc::m_arraySize - 1)
        unsigned int m_entrySize;   // 0x0c GetTableEntrySize(m_type, m_count)
        unsigned int m_dirty;       // 0x10 set when the entry is (re)initialised
        unsigned char* m_array;     // 0x14 m_arraySize * m_entrySize bytes

        inline EntryData()
        {
            m_type = Entry::Undefined;
            m_count = 0;
            m_arraySize = 0;
            m_entrySize = 0;
            m_dirty = 0;
            m_array = NULL;
        }

        inline ~EntryData()
        {
            if (m_array != NULL) {
                free(m_array);
                m_array = NULL;
            }
            m_type = Entry::Undefined;
            m_count = 0;
            m_arraySize = 0;
            m_entrySize = 0;
            m_dirty = 0;
            m_array = NULL;
        }
    };

    CTypedVariableTable();
    explicit CTypedVariableTable(unsigned int classId);

    inline virtual ~CTypedVariableTable()
    {
        Destroy();
    }

    /** Free everything (the table becomes empty). */
    void Destroy();
    /** Reset and preallocate room for `capacity` entries. Returns 1. */
    unsigned char Initialize(unsigned int capacity);
    /** Declare (or redeclare) a variable; it takes effect at the next sync. */
    void Add(const wchar_t* name, Entry::Type type, unsigned int count, unsigned int arraySize);
    /** Rebuild the packed table from the declared variables and run the sync handlers. */
    void SyncNumberOfEntriesAndTableSize();

    /** Bytes used by `count` elements of `type` (rounded up to 4 for booleans). */
    static unsigned int GetTableEntrySize(Entry::Type type, unsigned int count);

    /** Index of the entry called `name`, 0x7fffffff if there is none. */  // added by b037x
    inline int GetEntryIndex(const wchar_t* name)  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        for (int index = 0; index < m_entries.m_size; index++) {  // added by b037x
            const wchar_t* entryName = m_entries.m_data[index]->m_name.m_string;  // added by b037x
            if (entryName != NULL && wcscmp(name, entryName) == 0) {  // added by b037x
                return index;  // added by b037x
            }  // added by b037x
        }  // added by b037x
        return 0x7fffffff;  // added by b037x
    }  // added by b037x

    /** Index of the entry whose name equals `name` (NULL equals only NULL), 0x7fffffff if none. */  // added by b040x
    inline int GetEntryIndex(const CStrWChar& name)  // added by b040x
    {  // added by b040x
        const wchar_t* text = name.m_string;  // added by b040x
        if (m_dirty) {  // added by b040x
            SyncNumberOfEntriesAndTableSize();  // added by b040x
        }  // added by b040x
        for (int index = 0; index < m_entries.m_size; index++) {  // added by b040x
            const wchar_t* entryName = m_entries.m_data[index]->m_name.m_string;  // added by b040x
            if (text != NULL && entryName != NULL) {  // added by b040x
                if (wcscmp(text, entryName) == 0) {  // added by b040x
                    return index;  // added by b040x
                }  // added by b040x
            } else if (entryName == text) {  // added by b040x
                return index;  // added by b040x
            }  // added by b040x
        }  // added by b040x
        return 0x7fffffff;  // added by b040x
    }  // added by b040x

    /** Data of entry `index` in the packed table. */  // added by b037x
    inline unsigned char* GetEntryData(int index)  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        unsigned int offset = m_entryOffsets[index];  // Read the entry before adding its header.
        return m_table + (m_entryHeaderSize + offset);  // added by b037x
    }  // added by b037x

    /** Entries in the current table. */  // added by b037x
    inline unsigned int GetNumberOfEntries()  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        return m_entryCount;  // added by b037x
    }  // added by b037x

    /** Bytes of the packed table. */  // added by b037x
    inline unsigned int GetTableSize()  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        return m_tableSize;  // added by b037x
    }  // added by b037x

    /** The space reserved in front of the first entry (NULL when there is none). */  // added by b037x
    inline unsigned char* GetTableHeader()  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        unsigned char* header = NULL;  // added by b037x
        if (m_firstEntryOffset != 0) {  // added by b037x
            header = m_table;  // added by b037x
        }  // added by b037x
        return header;  // added by b037x
    }  // added by b037x

    /** The space reserved in front of entry `index` (NULL when there is none). */  // added by b037x
    inline unsigned char* GetEntryHeader(int index)  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        unsigned char* header = NULL;  // added by b037x
        if (m_entryHeaderSize != 0) {  // added by b037x
            header = m_table + m_entryOffsets[index];  // added by b037x
        }  // added by b037x
        return header;  // added by b037x
    }  // added by b037x

    /** Bytes of the data of entry `index`. */  // added by b037x
    inline unsigned int GetEntryDataSize(int index)  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        EntryDesc* desc = m_entries.m_data[index];  // added by b037x
        return GetTableEntrySize(desc->m_type, desc->m_count);  // added by b037x
    }  // added by b037x

    /** Name of entry `index`. */  // added by b037x
    inline const wchar_t* GetEntryName(int index)  // added by b037x
    {  // added by b037x
        if (m_dirty) {  // added by b037x
            SyncNumberOfEntriesAndTableSize();  // added by b037x
        }  // added by b037x
        return m_entries.m_data[index]->m_name.m_string;  // added by b037x
    }  // added by b037x

    /** Save the current value of entry `index` on its history stack (EntryData::m_dirty is the depth). */  // added by b037x
    inline void PushEntry(int index)  // added by b037x
    {  // added by b037x
        EntryData* data = &m_entryData[index];  // added by b037x
        memcpy(data->m_array + (data->m_dirty - 1) * data->m_entrySize,  // added by b037x
               m_table + (m_entryOffsets[index] + m_entryHeaderSize), data->m_entrySize);  // added by b037x
        data->m_dirty++;  // added by b037x
    }  // added by b037x

    /** Restore the value saved by the last PushEntry(index). */  // added by b039x
    inline void PopEntry(int index)  // added by b039x
    {  // added by b039x
        EntryData* data = &m_entryData[index];  // added by b039x
        data->m_dirty--;  // added by b039x
        memcpy(m_table + (m_entryOffsets[index] + m_entryHeaderSize),  // added by b039x
               data->m_array + (data->m_dirty - 1) * data->m_entrySize, data->m_entrySize);  // added by b039x
    }  // added by b039x

    int m_entryCount;                               // 0x08 entries in the current table
    core::CVector<EntryDesc*> m_pending;            // 0x0c declared since the last sync
    core::CVector<EntryDesc*> m_entries;            // 0x24 entries of the current table
    unsigned int m_tableSize;                       // 0x3c bytes in m_table
    unsigned int m_firstEntryOffset;                // 0x40 offset of the first entry
    unsigned int m_entryHeaderSize;                 // 0x44 bytes before each entry's data
    unsigned char* m_removed;                       // 0x48 per entry: redeclared, drop at next sync
    unsigned char m_dirty;                          // 0x4c declarations changed since the last sync
    core::CVector<SyncHandler> m_syncHandlers;      // 0x50
    void* m_reserved68;                             // 0x68 (only cleared by the default constructor)
    EntryData* m_entryData;                         // 0x6c one per entry
    unsigned int* m_entryOffsets;                   // 0x70 offset of each entry in m_table
    unsigned char* m_table;                         // 0x74 packed entry storage
};

} } } }

#endif
