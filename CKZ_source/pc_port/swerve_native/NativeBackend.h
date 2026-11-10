// Entry point of the native Swerve backend for code that does not include m3gcore headers.
#pragma once

struct ISwerve;

namespace ckz::native {

// Environment variable choosing the Swerve backend. The native m3gcore runtime is the default;
// "arm" selects the emulated original (Unicorn + swerve-arm.bin) for A/B comparison.
constexpr const char* BackendVariable = "CKZ_SWERVE";
constexpr const char* BackendArm = "arm";

/** Create the native runtime root object (swvNew of the native backend). Returns 0 on success. */
int CreateSwerve(ISwerve** swerve);

} // namespace ckz::native
