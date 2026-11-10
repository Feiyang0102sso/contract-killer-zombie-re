// Entry point of the emulated Swerve backend (Unicorn + swerve-arm.bin) for code that does not
// include Unicorn headers. swvNew calls it for Engine=swerve; the backend tests (tests/) create it
// next to the native runtime to compare both on the same input.
#pragma once

struct ISwerve;

namespace ckz::arm {

/** Create a root object of the original Swerve core (swvNew of the ARM backend). Returns 0 on success. */
int CreateSwerve(ISwerve** swerve);

} // namespace ckz::arm
