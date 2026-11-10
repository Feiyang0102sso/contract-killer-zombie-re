// Test: hot-spot profiler of the emulated ARM Swerve core (Engine=swerve in zombsniper.cfg).
// Compiled only into the test build (CkzTests=true). Set CKZ_ARM_PROFILE=1 before starting the
// game, then press F8 to start and stop recording; stopping writes arm_profile.txt and
// arm_callers.txt next to the executable.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <array>
#include <unicorn/unicorn.h>

namespace ckz::test {

constexpr const char* ArmProfileVariable = "CKZ_ARM_PROFILE";
constexpr const char* ArmProfileFileName = "arm_profile.txt";
constexpr const char* ArmCallersFileName = "arm_callers.txt";
constexpr size_t ArmCallerFrames = 8; // native frames kept per top-level Swerve call

/** Native return addresses above a top-level Swerve call, innermost first. */
using ArmCallerStack = std::array<uint32_t, ArmCallerFrames>;

/** State of one top-level Swerve call between BeginArmCall and EndArmCall. */
struct ArmCallSample {
    bool timed = false;
    double start = 0.0; // seconds
    uint64_t instructionsStart = 0;
    ArmCallerStack caller = {};
};

/** Install the block hook when CKZ_ARM_PROFILE is set; called once after the engine is set up. */
void InstallArmProfiler(uc_engine* engine, uint32_t codeStart, uint32_t codeEnd, uint32_t guestOffset);

/** Call at the start of an outermost Swerve call; nested calls are already inside its time. */
void BeginArmCall(ArmCallSample& sample);

/** Call at the end of the same Swerve call; does nothing when the call was not timed. */
void EndArmCall(const ArmCallSample& sample);

/** F8: start or stop recording; stopping writes the profile files. */
void ToggleArmProfile();

} // namespace ckz::test
