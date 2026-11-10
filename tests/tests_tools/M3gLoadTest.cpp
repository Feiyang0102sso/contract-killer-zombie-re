// M0 check of the native M3G engine: every M3G file of the game must load with m3gcore.
// Runs inside the game process after the GL context exists, because m3gcore may touch GL
// while it creates images and textures. See PORT_PLAN.md, P2 / M0.
#include "M3gLoadTest.h"
#include "../../CKZ_source/pc_port/swerve_native/NativeRuntime.h"
#include <m3g_core.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace ckz::m3g {
namespace {

constexpr const char* M3gExtension = ".m3g";

// m3gcore class ids (M3G_CLASS_*) to names, for the per-file summary.
const std::map<M3GClass, const char*> ClassNames = {
    { M3G_CLASS_ANIMATION_CONTROLLER, "AnimationController" },
    { M3G_CLASS_ANIMATION_TRACK, "AnimationTrack" },
    { M3G_CLASS_APPEARANCE, "Appearance" },
    { M3G_CLASS_BACKGROUND, "Background" },
    { M3G_CLASS_CAMERA, "Camera" },
    { M3G_CLASS_COMPOSITING_MODE, "CompositingMode" },
    { M3G_CLASS_FOG, "Fog" },
    { M3G_CLASS_GROUP, "Group" },
    { M3G_CLASS_IMAGE, "Image2D" },
    { M3G_CLASS_INDEX_BUFFER, "IndexBuffer" },
    { M3G_CLASS_KEYFRAME_SEQUENCE, "KeyframeSequence" },
    { M3G_CLASS_LIGHT, "Light" },
    { M3G_CLASS_MATERIAL, "Material" },
    { M3G_CLASS_MESH, "Mesh" },
    { M3G_CLASS_POLYGON_MODE, "PolygonMode" },
    { M3G_CLASS_SKINNED_MESH, "SkinnedMesh" },
    { M3G_CLASS_SPRITE, "Sprite3D" },
    { M3G_CLASS_TEXTURE, "Texture2D" },
    { M3G_CLASS_VERTEX_ARRAY, "VertexArray" },
    { M3G_CLASS_VERTEX_BUFFER, "VertexBuffer" },
    { M3G_CLASS_WORLD, "World" },
};

M3Genum lastError = M3G_NO_ERROR;

void* Allocate(M3Gpointer bytes)
{
    return malloc(static_cast<size_t>(bytes));
}

void Release(void* pointer)
{
    free(pointer);
}

void RememberError(M3Genum errorCode, M3GInterface)
{
    lastError = errorCode;
}

const char* ClassName(M3GClass objectClass)
{
    const auto found = ClassNames.find(objectClass);
    if (found == ClassNames.end()) {
        return "?";
    }
    return found->second;
}

/** Load one file in a single m3gDecodeData call; print the root objects. Returns true on success. */
bool LoadFile(M3GInterface m3g, const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    std::vector<M3Gubyte> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    // Swerve's extended files go through the same conversion as in the native loader.
    if (ckz::native::IsSwerveExtendedM3g(bytes)) {
        std::vector<M3Gubyte> standard;
        if (!ckz::native::ConvertSwerveExtendedM3g(bytes, standard)) {
            fprintf(stderr, "M3G load FAIL %-34s extended file conversion failed\n", path.filename().string().c_str());
            return false;
        }
        bytes.swap(standard);
    }

    lastError = M3G_NO_ERROR;
    const Uint64 start = SDL_GetTicksNS();
    M3GLoader loader = m3gCreateLoader(m3g);
    const M3Gsizei stillNeeded = m3gDecodeData(loader, static_cast<M3Gsizei>(bytes.size()), bytes.data());
    const double milliseconds = static_cast<double>(SDL_GetTicksNS() - start) / 1e6;

    const M3Gint rootCount = m3gGetLoadedObjects(loader, nullptr);
    bool loaded = lastError == M3G_NO_ERROR && stillNeeded == 0 && rootCount > 0;

    std::map<std::string, int> rootClasses;
    if (rootCount > 0) {
        std::vector<M3Gulong> roots(static_cast<size_t>(rootCount));
        m3gGetLoadedObjects(loader, roots.data());
        for (M3Gulong root : roots) {
            const auto object = reinterpret_cast<M3GObject>(static_cast<uintptr_t>(root));
            rootClasses[ClassName(m3gGetClass(object))]++;
        }
    }

    std::string summary;
    for (const auto& [name, count] : rootClasses) {
        summary += " " + name + "x" + std::to_string(count);
    }
    const char* status = "OK  ";
    if (!loaded) {
        status = "FAIL";
    }
    fprintf(stderr, "M3G load %s %-34s %9zu bytes %8.1f ms  error 0x%02x  need %d  roots%s\n", status,
            path.filename().string().c_str(), bytes.size(), milliseconds, lastError, stillNeeded, summary.c_str());

    // Deleting the loader releases the objects it holds.
    m3gDeleteObject(reinterpret_cast<M3GObject>(loader));
    return loaded;
}

} // namespace

int RunLoadTest(const char* folder)
{
    M3Gparams params = {};
    params.mallocFunc = Allocate;
    params.freeFunc = Release;
    params.errorFunc = RememberError;
    M3GInterface m3g = m3gCreateInterface(&params);
    if (m3g == nullptr) {
        fprintf(stderr, "M3G load test: m3gCreateInterface failed\n");
        return 1;
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(folder))) {
        if (entry.path().extension() == M3gExtension) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());

    int failed = 0;
    for (const auto& path : files) {
        if (!LoadFile(m3g, path)) {
            failed++;
        }
    }
    fprintf(stderr, "M3G load test: %zu files, %d failed\n", files.size(), failed);
    m3gDeleteInterface(m3g);
    if (failed != 0) {
        return 1;
    }
    return 0;
}

} // namespace ckz::m3g
