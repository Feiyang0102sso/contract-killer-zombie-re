// Test: hot-spot profiler of the emulated ARM Swerve core, see ArmProfiler.h.
#include "ArmProfiler.h"
#include "../../CKZ_source/pc_port/swerve_compat/ArmCall.h"
#include <windows.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <algorithm>
#include <filesystem>
#include <map>
#include <vector>

namespace ckz::test {
namespace {

/** Cost of all top-level Swerve calls made from one native call stack. */
struct CallerCost {
    uint64_t calls = 0;
    double seconds = 0.0;
    uint64_t instructions = 0; // guest instructions executed, nested calls included
};

/** Per guest basic block counters, indexed by (block address - codeStart) / 4. */
struct BlockProfile {
    uint32_t codeStart = 0;
    size_t codeWords = 0;
    uint32_t guestOffset = 0;
    std::vector<uint64_t> hits;
    std::vector<uint32_t> instructions; // instructions in the block
    std::vector<uint32_t> floatInstructions; // VFP/NEON instructions in the block
    bool recording = false;
    double recordStart = 0.0; // seconds
    double swerveSeconds = 0.0; // wall time spent inside top-level Swerve calls
    uint64_t topLevelCalls = 0;
    uint64_t executedInstructions = 0; // running total, used to cost each call
    std::map<ArmCallerStack, CallerCost> callers;
};

BlockProfile* profile; // null unless CKZ_ARM_PROFILE is set and the ARM backend runs

double NowSeconds()
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
}

/** True for ARM-mode VFP and NEON instructions, the ones QEMU runs through softfloat helpers. */
bool IsFloatInstruction(uint32_t instruction)
{
    const uint32_t condition = instruction >> 28;
    if (condition == 0xf) {
        // NEON data processing (0xf2/0xf3) and element load/store (0xf4).
        const uint32_t top = instruction >> 24;
        return top == 0xf2 || top == 0xf3 || top == 0xf4;
    }
    // Coprocessor space (bits 27..25 = 110 or 111) using coprocessor 10 or 11.
    const uint32_t group = (instruction >> 25) & 7;
    const uint32_t coprocessor = (instruction >> 8) & 0xe;
    return (group == 6 || group == 7) && coprocessor == 0xa;
}

void CountBlock(uc_engine*, uint64_t address, uint32_t size, void*)
{
    if (!profile->recording) {
        return;
    }
    const size_t index = static_cast<size_t>(address - profile->codeStart) / 4;
    if (index >= profile->codeWords) {
        return;
    }
    if (profile->instructions[index] == 0) {
        // First time this block runs: classify its instructions once.
        const auto* words = static_cast<const uint32_t*>(arm::ResolvePointer(static_cast<uint32_t>(address)));
        const uint32_t count = size / 4;
        uint32_t floats = 0;
        for (uint32_t offset = 0; offset < count; offset++) {
            if (IsFloatInstruction(words[offset])) {
                floats++;
            }
        }
        profile->instructions[index] = count;
        profile->floatInstructions[index] = floats;
    }
    profile->hits[index]++;
    profile->executedInstructions += profile->instructions[index];
}

/** Write the native callers of Swerve, costliest first, to arm_callers.txt. */
void WriteCallers()
{
    std::vector<std::pair<ArmCallerStack, CallerCost>> sorted(profile->callers.begin(), profile->callers.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
        return left.second.seconds > right.second.seconds;
    });

    const auto path = std::filesystem::u8path(SDL_GetBasePath()) / ArmCallersFileName;
    FILE* file = _wfopen(path.c_str(), L"w");
    if (file == nullptr) {
        fwprintf(stderr, L"Could not write ARM caller profile: %ls\n", path.c_str());
        return;
    }
    fprintf(file, "# module_base %08x\n", reinterpret_cast<uint32_t>(GetModuleHandleW(nullptr)));
    fprintf(file, "# seconds calls guest_instructions native_return_addresses...\n");
    for (const auto& [stack, cost] : sorted) {
        fprintf(file, "%.6f %llu %llu", cost.seconds, static_cast<unsigned long long>(cost.calls),
                static_cast<unsigned long long>(cost.instructions));
        for (uint32_t address : stack) {
            if (address != 0) {
                fprintf(file, " %08x", address);
            }
        }
        fprintf(file, "\n");
    }
    fclose(file);
    fwprintf(stderr, L"ARM caller profile written: %ls (%zu stacks)\n", path.c_str(), sorted.size());
}

/** Write every executed block, hottest first, to arm_profile.txt next to the executable. */
void WriteProfile(double recordedSeconds)
{
    std::vector<size_t> executed;
    for (size_t index = 0; index < profile->codeWords; index++) {
        if (profile->hits[index] != 0) {
            executed.push_back(index);
        }
    }
    std::sort(executed.begin(), executed.end(), [](size_t left, size_t right) {
        return profile->hits[left] * profile->instructions[left] > profile->hits[right] * profile->instructions[right];
    });

    const auto path = std::filesystem::u8path(SDL_GetBasePath()) / ArmProfileFileName;
    FILE* file = _wfopen(path.c_str(), L"w");
    if (file == nullptr) {
        fwprintf(stderr, L"Could not write ARM profile: %ls\n", path.c_str());
        return;
    }
    fprintf(file, "# seconds %.3f swerve_seconds %.3f top_level_calls %llu\n", recordedSeconds,
            profile->swerveSeconds, static_cast<unsigned long long>(profile->topLevelCalls));
    fprintf(file, "# address hits instructions float_instructions (original address = address - %08x)\n",
            profile->guestOffset);
    for (size_t index : executed) {
        const uint32_t address = profile->codeStart + static_cast<uint32_t>(index) * 4;
        fprintf(file, "%08x %llu %u %u\n", address, static_cast<unsigned long long>(profile->hits[index]),
                profile->instructions[index], profile->floatInstructions[index]);
    }
    fclose(file);
    fwprintf(stderr, L"ARM profile written: %ls (%zu blocks)\n", path.c_str(), executed.size());
}

} // namespace

void InstallArmProfiler(uc_engine* engine, uint32_t codeStart, uint32_t codeEnd, uint32_t guestOffset)
{
    if (SDL_getenv(ArmProfileVariable) == nullptr) {
        return;
    }
    // The block hook slows emulation down, so it is only installed on request.
    profile = new BlockProfile();
    profile->codeStart = codeStart;
    profile->codeWords = (codeEnd - codeStart + 3) / 4;
    profile->guestOffset = guestOffset;
    profile->hits.resize(profile->codeWords);
    profile->instructions.resize(profile->codeWords);
    profile->floatInstructions.resize(profile->codeWords);
    uc_hook hook;
    const uc_err status = uc_hook_add(engine, &hook, UC_HOOK_BLOCK, reinterpret_cast<void*>(CountBlock), nullptr,
                                      codeStart, codeEnd - 1);
    if (status != UC_ERR_OK) {
        fprintf(stderr, "ARM profiler hook failed: %s\n", uc_strerror(status));
        delete profile;
        profile = nullptr;
        return;
    }
    fprintf(stderr, "Swerve ARM profiler ready: press F8 to start and stop recording\n");
}

void BeginArmCall(ArmCallSample& sample)
{
    if (profile == nullptr || !profile->recording) {
        return;
    }
    // Skip this function and arm::Call, so the stack starts at the caller of Swerve.
    void* frames[ArmCallerFrames] = {};
    CaptureStackBackTrace(2, ArmCallerFrames, frames, nullptr);
    for (size_t index = 0; index < ArmCallerFrames; index++) {
        sample.caller[index] = reinterpret_cast<uint32_t>(frames[index]);
    }
    sample.timed = true;
    sample.instructionsStart = profile->executedInstructions;
    sample.start = NowSeconds();
    profile->topLevelCalls++;
}

void EndArmCall(const ArmCallSample& sample)
{
    if (!sample.timed) {
        return;
    }
    const double seconds = NowSeconds() - sample.start;
    profile->swerveSeconds += seconds;
    CallerCost& cost = profile->callers[sample.caller];
    cost.calls++;
    cost.seconds += seconds;
    cost.instructions += profile->executedInstructions - sample.instructionsStart;
}

void ToggleArmProfile()
{
    if (profile == nullptr) {
        fprintf(stderr, "ARM profiler is off; set %s=1 and Engine=swerve in zombsniper.cfg before starting the game\n",
                ArmProfileVariable);
        return;
    }
    if (!profile->recording) {
        std::fill(profile->hits.begin(), profile->hits.end(), 0);
        profile->swerveSeconds = 0.0;
        profile->topLevelCalls = 0;
        profile->callers.clear();
        profile->recordStart = NowSeconds();
        profile->recording = true;
        fprintf(stderr, "ARM profile recording started\n");
        return;
    }
    profile->recording = false;
    const double recordedSeconds = NowSeconds() - profile->recordStart;
    fprintf(stderr, "ARM profile recording stopped: %.1f s, %.1f s inside Swerve\n", recordedSeconds,
            profile->swerveSeconds);
    WriteProfile(recordedSeconds);
    WriteCallers();
}

} // namespace ckz::test
