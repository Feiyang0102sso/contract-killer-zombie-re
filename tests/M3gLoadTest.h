// Test: M0 check of the native M3G engine (PORT_PLAN.md, P2): load M3G files with m3gcore.
// Compiled only into the test build (CkzTests=true).
#pragma once

namespace ckz::m3g {

// Set to a folder of .m3g files to run the load test instead of the game; the process then exits.
constexpr const char* LoadTestVariable = "CKZ_M3G_LOADTEST";

/** Load every .m3g file in `folder` with m3gcore and print one line per file. Returns 0 if all loaded. */
int RunLoadTest(const char* folder);

} // namespace ckz::m3g
