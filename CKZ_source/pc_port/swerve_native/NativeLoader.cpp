// Native Swerve loader (ILoader of SwerveApi.h): CSwerveLoader feeds an M3G file in chunks
// (Begin, Decode..., End) and then reads the root objects. m3gcore decodes the whole file in End.
// The game's M3G files have no external references, so the XREF queries always report none.
#include "NativeRuntime.h"
#include <stdio.h>
#include <string.h>
#include <vector>

namespace ckz::native {
namespace {

/** GetXREFURI length meaning "no pending external reference" (CSwerveLoader::processXREFs). */
constexpr int NoPendingXref = -1;

struct LoaderObject : Object {
    std::vector<M3Gubyte> data;
    std::vector<M3GObject> roots; // each holds one m3gcore reference
};

LoaderObject* AsLoader(Object* object)
{
    return static_cast<LoaderObject*>(object);
}

void DropRoots(LoaderObject* loader)
{
    for (M3GObject root : loader->roots) {
        m3gDeleteRef(root);
    }
    loader->roots.clear();
}

void DestroyLoader(Object* object)
{
    LoaderObject* loader = AsLoader(object);
    DropRoots(loader);
    delete loader;
}

/** Copy the user parameters of every loaded object out of the m3gcore loader. */
void HarvestUserParameters(M3GLoader loader)
{
    const M3Gint objectCount = m3gGetObjectsWithUserParameters(loader, nullptr);
    if (objectCount <= 0) {
        return;
    }
    std::vector<M3Gulong> objects(static_cast<size_t>(objectCount));
    m3gGetObjectsWithUserParameters(loader, objects.data());
    for (M3Gint objectIndex = 0; objectIndex < objectCount; objectIndex++) {
        const auto object = reinterpret_cast<M3GObject>(static_cast<uintptr_t>(objects[objectIndex]));
        // Keep the object alive so its address cannot be reused by another object with other parameters.
        m3gAddRef(object);
        const M3Gint parameterCount = m3gGetNumUserParameters(loader, objectIndex);
        for (M3Gint parameter = 0; parameter < parameterCount; parameter++) {
            const M3Gint length = m3gGetUserParameter(loader, objectIndex, parameter, nullptr);
            std::vector<M3Gbyte> bytes(static_cast<size_t>(length) + 1);
            const M3Gint id = m3gGetUserParameter(loader, objectIndex, parameter, bytes.data());
            StoreUserParameter(object, id, bytes.data(), length);
        }
    }
}

/** Diagnostics: a file that decoded without error but produced no root object. */
void DumpFailedLoad(const std::vector<M3Gubyte>& data, M3Gsizei stillNeeded)
{
    static int failures = 0;
    char name[64];
    snprintf(name, sizeof(name), "native_noroots_%d.m3g", failures);
    failures++;
    char head[64] = "";
    for (size_t index = 0; index < data.size() && index < 12; index++) {
        char text[4];
        snprintf(text, sizeof(text), "%02x ", data[index]);
        strcat(head, text);
    }
    fprintf(stderr, "Swerve native: no root objects in %zu bytes (still needed %d), first bytes %s; saved %s\n",
            data.size(), stillNeeded, head, name);
    FILE* file = fopen(name, "wb");
    if (file != nullptr) {
        fwrite(data.data(), 1, data.size(), file);
        fclose(file);
    }
}

int LoaderBegin(Object* self)
{
    LoaderObject* loader = AsLoader(self);
    loader->data.clear();
    DropRoots(loader);
    return ErrorNone;
}

/** Take `length` bytes at `offset` of `buffer`; everything is consumed. */
int LoaderDecode(Object* self, int, const unsigned char* buffer, int offset, int length, int* consumed)
{
    LoaderObject* loader = AsLoader(self);
    if (length > 0) {
        loader->data.insert(loader->data.end(), buffer + offset, buffer + offset + length);
    }
    if (consumed != nullptr) {
        *consumed = length;
    }
    return ErrorNone;
}

int LoaderEnd(Object* self)
{
    LoaderObject* loader = AsLoader(self);
    TakeError();
    if (IsSwerveExtendedM3g(loader->data)) {
        std::vector<M3Gubyte> standard;
        if (!ConvertSwerveExtendedM3g(loader->data, standard)) {
            fprintf(stderr, "Swerve native: extended M3G file of %zu bytes could not be converted\n", loader->data.size());
            loader->data.clear();
            return ErrorCorruptFile;
        }
        loader->data.swap(standard);
    }
    M3GLoader m3gLoader = m3gCreateLoader(Interface());
    const M3Gsizei stillNeeded = m3gDecodeData(m3gLoader, static_cast<M3Gsizei>(loader->data.size()), loader->data.data());
    int error = TakeError();
    if (error == ErrorNone && stillNeeded != 0) {
        fprintf(stderr, "Swerve native: M3G file incomplete, %d more bytes expected\n", stillNeeded);
        error = ErrorCorruptFile;
    }
    if (error == ErrorNone) {
        const M3Gint rootCount = m3gGetLoadedObjects(m3gLoader, nullptr);
        std::vector<M3Gulong> roots(static_cast<size_t>(rootCount) + 1);
        m3gGetLoadedObjects(m3gLoader, roots.data());
        for (M3Gint index = 0; index < rootCount; index++) {
            const auto root = reinterpret_cast<M3GObject>(static_cast<uintptr_t>(roots[index]));
            m3gAddRef(root);
            loader->roots.push_back(root);
        }
        HarvestUserParameters(m3gLoader);
        if (rootCount == 0) {
            DumpFailedLoad(loader->data, stillNeeded);
        }
    } else {
        error = ErrorCorruptFile;
    }
    m3gDeleteObject(reinterpret_cast<M3GObject>(m3gLoader));
    loader->data.clear();
    loader->data.shrink_to_fit();
    return error;
}

int LoaderGetObjectCount(Object* self, int* count)
{
    *count = static_cast<int>(AsLoader(self)->roots.size());
    return ErrorNone;
}

int LoaderGetObject(Object* self, int index, void** object)
{
    LoaderObject* loader = AsLoader(self);
    if (index < 0 || index >= static_cast<int>(loader->roots.size())) {
        *object = nullptr;
        return ErrorInvalidParameter;
    }
    WrapInto(loader->roots[static_cast<size_t>(index)], object);
    return ErrorNone;
}

int LoaderGetXrefUri(Object*, int, unsigned char*, int* length)
{
    *length = NoPendingXref;
    return ErrorNone;
}

/** XREF queries are only valid while one is pending, which never happens here. */
int LoaderNoXref(Object*)
{
    fprintf(stderr, "Swerve native: XREF query without a pending XREF\n");
    return ErrorInvalidState;
}

} // namespace

Object* CreateLoader()
{
    LoaderObject* loader = new LoaderObject();
    InitObject(loader, ClassLoader);
    return loader;
}

void FillLoaderSlots(ObjectClass& objectClass)
{
    FillQueryInterfaceSlots(objectClass);
    objectClass.destroy = DestroyLoader;
    SetSlot(objectClass, 3, &LoaderBegin);            // 0x0c
    SetSlot(objectClass, 4, &LoaderEnd);              // 0x10
    SetSlot(objectClass, 5, &LoaderDecode);           // 0x14
    SetSlot(objectClass, 6, &LoaderGetObjectCount);   // 0x18
    SetSlot(objectClass, 7, &LoaderGetObject);        // 0x1c
    SetSlot(objectClass, 8, &LoaderNoXref);           // 0x20 GetXREFType
    SetSlot(objectClass, 9, &LoaderGetXrefUri);       // 0x24
    SetSlot(objectClass, 10, &LoaderNoXref);          // 0x28 GetXREFUserIDs
    SetSlot(objectClass, 11, &LoaderNoXref);          // 0x2c ResolveXREF
    SetSlot(objectClass, 12, &LoaderNoXref);          // 0x30 GetXREFImageFormat
}

} // namespace ckz::native
