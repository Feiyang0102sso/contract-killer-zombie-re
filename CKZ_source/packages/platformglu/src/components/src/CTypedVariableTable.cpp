// CTypedVariableTable.cpp - packed block of typed variables.
#include "CTypedVariableTable.h"
#include "CStdUtil_Cocoa.h"

namespace com { namespace glu { namespace platform { namespace components {

// type names accepted by Entry::GetType
static const wchar_t CVT_Entry_Undefined_str[] = L"undefined";
static const wchar_t CVT_Entry_Boolean_str[] = L"boolean";
static const wchar_t CVT_Entry_Boolean2d_str[] = L"boolean2d";
static const wchar_t CVT_Entry_Boolean3d_str[] = L"boolean3d";
static const wchar_t CVT_Entry_Boolean4d_str[] = L"boolean4d";
static const wchar_t CVT_Entry_Float_str[] = L"float";
static const wchar_t CVT_Entry_Integer_str[] = L"integer";
static const wchar_t CVT_Entry_Integer2d_str[] = L"integer2d";
static const wchar_t CVT_Entry_Integer3d_str[] = L"integer3d";
static const wchar_t CVT_Entry_Integer4d_str[] = L"integer4d";
static const wchar_t CVT_Entry_Matrix2d_str[] = L"matrix2d";
static const wchar_t CVT_Entry_Matrix3d_str[] = L"matrix3d";
static const wchar_t CVT_Entry_Matrix4d_str[] = L"matrix4d";
static const wchar_t CVT_Entry_Matrix2dx_str[] = L"matrix2dx";
static const wchar_t CVT_Entry_Matrix3dx_str[] = L"matrix3dx";
static const wchar_t CVT_Entry_Matrix4dx_str[] = L"matrix4dx";
static const wchar_t CVT_Entry_Sampler2d_str[] = L"sampler2d";
static const wchar_t CVT_Entry_SamplerCube_str[] = L"samplercube";
static const wchar_t CVT_Entry_Vector2d_str[] = L"vector2d";
static const wchar_t CVT_Entry_Vector3d_str[] = L"vector3d";
static const wchar_t CVT_Entry_Vector4d_str[] = L"vector4d";

/** Initial offsets of a freshly initialised table. */
static const unsigned int kDefaultEntryOffset = 16;
/** Sync handler slots preallocated by Initialize. */
static const int kSyncHandlerCapacity = 10;

void CTypedVariableTable::Destroy()
{
    if (m_entryData != NULL) {
        delete[] m_entryData;
        m_entryData = NULL;
    }
    if (m_table != NULL) {
        delete[] m_table;
        m_table = NULL;
    }
    if (m_entryOffsets != NULL) {
        delete[] m_entryOffsets;
        m_entryOffsets = NULL;
    }
    if (m_removed != NULL) {
        delete[] m_removed;
        m_removed = NULL;
    }

    for (int index = 0; index < m_pending.Size(); index++) {
        delete m_pending[index];
    }
    for (int index = 0; index < m_entries.Size(); index++) {
        delete m_entries[index];
    }

    m_tableSize = 0;
    m_pending.Clear();
    m_entries.Clear();
    m_syncHandlers.Clear();
    m_entryHeaderSize = 0;
    m_entryCount = 0;
}

CTypedVariableTable::Entry::Type CTypedVariableTable::Entry::GetType(const wchar_t* name)
{
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Undefined_str) == 0) {
        return Undefined;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Boolean_str) == 0) {
        return Boolean;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Boolean2d_str) == 0) {
        return Boolean2d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Boolean3d_str) == 0) {
        return Boolean3d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Boolean4d_str) == 0) {
        return Boolean4d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Float_str) == 0) {
        return Float;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Integer_str) == 0) {
        return Integer;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Integer2d_str) == 0) {
        return Integer2d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Integer3d_str) == 0) {
        return Integer3d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Integer4d_str) == 0) {
        return Integer4d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix2d_str) == 0) {
        return Matrix2d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix3d_str) == 0) {
        return Matrix3d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix4d_str) == 0) {
        return Matrix4d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix2dx_str) == 0) {
        return Matrix2dx;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix3dx_str) == 0) {
        return Matrix3dx;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Matrix4dx_str) == 0) {
        return Matrix4dx;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Sampler2d_str) == 0) {
        return Sampler2d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_SamplerCube_str) == 0) {
        return SamplerCube;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Vector2d_str) == 0) {
        return Vector2d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Vector3d_str) == 0) {
        return Vector3d;
    }
    if (CStdUtil_Cocoa::WcsICmp(name, CVT_Entry_Vector4d_str) == 0) {
        return Vector4d;
    }
    return Undefined;
}

/** `bytes` rounded up to a multiple of 4. */
static inline unsigned int AlignTo4(unsigned int bytes)
{
    return (4 - bytes % 4) % 4 + bytes;
}

unsigned int CTypedVariableTable::GetTableEntrySize(Entry::Type type, unsigned int count)
{
    switch (type) {
    case Entry::Boolean:
        return AlignTo4(count);
    case Entry::Boolean2d:
        return AlignTo4(count * 2);
    case Entry::Boolean3d:
        return AlignTo4(count * 3);
    case Entry::Boolean4d:
        return count * 4;
    case Entry::Float:
        return count * 4;
    case Entry::Integer:
        return count * 4;
    case Entry::Integer2d:
        return count * 8;
    case Entry::Integer3d:
        return count * 12;
    case Entry::Integer4d:
        return count * 16;
    case Entry::Matrix2d:
        return count * 24;
    case Entry::Matrix3d:
        return count * 72;
    case Entry::Matrix4d:
        return count * 72;
    case Entry::Matrix2dx:
        return count * 24;
    case Entry::Matrix3dx:
        return count * 44;
    case Entry::Matrix4dx:
        return count * 72;
    case Entry::Sampler2d:
        return count * 8;
    case Entry::SamplerCube:
        return count * 12;
    case Entry::Vector2d:
        return count * 8;
    case Entry::Vector3d:
        return count * 12;
    case Entry::Vector4d:
        return count * 16;
    default:
        return 0;
    }
}

CTypedVariableTable::CTypedVariableTable()
    : CClass(ClassId)
{
    m_entryCount = 0;
    m_tableSize = 0;
    m_firstEntryOffset = 0;
    m_entryHeaderSize = 0;
    m_removed = NULL;
    m_dirty = 0;
    m_reserved68 = NULL;
    m_entryData = NULL;
    m_entryOffsets = NULL;
    m_table = NULL;
}

CTypedVariableTable::CTypedVariableTable(unsigned int classId)
    : CClass(classId)
{
    m_entryCount = 0;
    m_tableSize = 0;
    m_firstEntryOffset = 0;
    m_entryHeaderSize = 0;
    m_removed = NULL;
    m_dirty = 0;
    m_entryData = NULL;
    m_entryOffsets = NULL;
    m_table = NULL;
}

unsigned char CTypedVariableTable::Initialize(unsigned int capacity)
{
    Destroy();
    m_pending.EnsureCapacity(capacity);
    m_entries.EnsureCapacity(capacity);
    m_firstEntryOffset = kDefaultEntryOffset;
    m_entryHeaderSize = kDefaultEntryOffset;
    m_syncHandlers.EnsureCapacity(kSyncHandlerCapacity);
    return 1;
}

void CTypedVariableTable::Add(const wchar_t* name, Entry::Type type, unsigned int count, unsigned int arraySize)
{
    // already part of the table: keep it if nothing changed, else drop it at the next sync
    int index = -1;
    for (int search = 0; search < m_entries.Size(); search++) {
        if (m_entries[search]->m_name == name) {
            index = search;
            break;
        }
    }
    if (index >= 0) {
        EntryDesc* existing = m_entries[index];
        if (existing->m_type == type && existing->m_count == count && existing->m_arraySize == arraySize) {
            m_removed[index] = 0;
            return;
        }
        m_removed[index] = 1;
    }

    m_dirty = 1;

    // already declared since the last sync: update it and move it to the end
    index = -1;
    for (int search = 0; search < m_pending.Size(); search++) {
        if (m_pending[search]->m_name == name) {
            index = search;
            break;
        }
    }
    if (index >= 0) {
        EntryDesc* pending = m_pending[index];
        // note: the array size is compared against m_entries[index], not against `pending`
        if (pending->m_type == type && pending->m_count == count && m_entries[index]->m_arraySize == arraySize) {
            return;
        }
        pending->m_name = name;
        pending->m_type = type;
        pending->m_count = count;
        m_pending.RemoveRange(index, index + 1);
        m_pending.Add(pending);
        return;
    }

    EntryDesc* declared = new EntryDesc;
    declared->m_name = name;
    declared->m_type = type;
    declared->m_count = count;
    declared->m_arraySize = arraySize;
    m_pending.Add(declared);
}

void CTypedVariableTable::SyncNumberOfEntriesAndTableSize()
{
    delete[] m_entryData;

    unsigned int* oldOffsets = m_entryOffsets;
    unsigned char* oldTable = m_table;

    // surviving entries plus the newly declared ones
    int keptCount = 0;
    for (int index = 0; index < m_entries.Size(); index++) {
        if (m_removed[index] == 0) {
            keptCount++;
        }
    }
    int newCount = m_pending.Size() + keptCount;
    m_entryOffsets = new unsigned int[newCount];

    unsigned int offset = m_firstEntryOffset;
    int slot = 0;
    for (int index = 0; index < m_entries.Size(); index++) {
        if (m_removed[index] == 0) {
            m_entryOffsets[slot] = offset;
            slot++;
            offset = m_entryHeaderSize + offset + GetTableEntrySize(m_entries[index]->m_type, m_entries[index]->m_count);
        }
    }
    for (int index = 0; index < m_pending.Size(); index++) {
        m_entries.Add(m_pending[index]);
        m_entryOffsets[slot + index] = offset;
        // note: the count is taken from m_entries[index], the type from the pending entry
        offset = m_entryHeaderSize + offset + GetTableEntrySize(m_pending[index]->m_type, m_entries[index]->m_count);
    }
    m_pending.Clear();

    // move the data of the surviving entries into the new table
    m_table = new unsigned char[offset];
    slot = 0;
    for (int index = 0; index < m_entryCount; index++) {
        if (m_removed[index] == 0) {
            memcpy(m_table + (m_entryHeaderSize + m_entryOffsets[slot]), oldTable + (oldOffsets[index] + m_entryHeaderSize),
                   GetTableEntrySize(m_entries[index]->m_type, m_entries[index]->m_count));
            slot++;
        }
    }
    if (oldTable != NULL) {
        delete[] oldTable;
    }
    if (oldOffsets != NULL) {
        delete[] oldOffsets;
    }
    if (m_removed != NULL) {
        delete[] m_removed;
    }

    m_tableSize = offset;
    m_removed = new unsigned char[newCount];
    for (int index = 0; index < newCount; index++) {
        m_removed[index] = 0;
    }

    m_entryData = new EntryData[newCount];
    for (int index = 0; index < newCount; index++) {
        EntryDesc* desc = m_entries[index];
        Entry::Type type = desc->m_type;
        unsigned int entryCount = desc->m_count;
        unsigned int arraySize = 0;
        if (desc->m_arraySize != 0) {
            arraySize = desc->m_arraySize - 1;
        }

        EntryData& data = m_entryData[index];
        if (data.m_array != NULL) {
            free(data.m_array);
            data.m_array = NULL;
        }
        data.m_array = NULL;
        data.m_type = type;
        data.m_count = entryCount;
        data.m_arraySize = arraySize;
        data.m_entrySize = GetTableEntrySize(type, entryCount);
        data.m_dirty = 1;
        if (arraySize != 0) {
            data.m_array = (unsigned char*)malloc(arraySize * data.m_entrySize);
        }
    }

    m_entryCount = newCount;
    m_dirty = 0;
    for (int index = 0; index < m_syncHandlers.Size(); index++) {
        m_syncHandlers[index].m_callback(m_syncHandlers[index].m_context, this);
    }
}

} } } }
