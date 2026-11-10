// Entry point of the native Swerve backend for code that does not include m3gcore headers.
// Engine in zombsniper.cfg chooses the backend: the native m3gcore runtime is the default;
// "swerve" selects the emulated original (Unicorn + swerve-arm.bin) for A/B comparison.
#pragma once

struct ISwerve;

namespace ckz::native {

/** Create the native runtime root object (swvNew of the native backend). Returns 0 on success. */
int CreateSwerve(ISwerve** swerve);

} // namespace ckz::native
