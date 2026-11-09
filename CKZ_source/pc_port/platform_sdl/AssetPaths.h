// PC resource layout: the single place that names every folder and file the game needs on disk.
// Startup checks the whole RequiredFiles table and reports each missing file by name.
#pragma once

namespace ckz {

/** Where a required file lives. */
enum class FileLocation {
    AssetsFolder,     // copied by the player from the original game package
    ExecutableFolder, // placed next to ckz_pc.exe by the build
};

struct RequiredFile {
    const wchar_t* name;
    FileLocation location;
    const char* purpose; // shown in the error message when the file is missing
};

// Both folders sit next to ckz_pc.exe unless the command line overrides them:
//   ckz_pc.exe [assets folder] [saves folder]
constexpr wchar_t AssetsFolderName[] = L"assets";
constexpr wchar_t SavesFolderName[] = L"saves";

// The Swerve image is cut from the original binary and staged by the build (scripts/stage_assets.py),
// so it stays with the executable and does not move when the assets folder is overridden.
constexpr wchar_t SwerveImageName[] = L"swerve-arm.bin";

constexpr RequiredFile RequiredFiles[] = {
    { L"zombsniper.big", FileLocation::AssetsFolder, "game data archive" },
    { L"vga.big", FileLocation::AssetsFolder, "1024x768 graphics archive" },
    { L"en.big", FileLocation::AssetsFolder, "English text archive" },
    { L"res_map.dat", FileLocation::AssetsFolder, "resource name map" },
    { L"ckz_intro_scene_1024.m4v", FileLocation::AssetsFolder, "intro movie" },
    { SwerveImageName, FileLocation::ExecutableFolder, "Swerve 3D engine image, staged by the build" },
};

} // namespace ckz
