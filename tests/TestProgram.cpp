// Entry of the test program, see TestProgram.h.
#include "TestProgram.h"
#include "GameHarness.h"
#include "tests_tools/M3gLoadTest.h"
#include "../CKZ_source/pc_port/platform_sdl/Config.h"
#include "../CKZ_source/pc_port/platform_sdl/Host.h"
#include <gtest/gtest.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>

namespace ckz::test {
namespace {

// OpenAL Soft reads this variable when it opens its first device: "null" plays nothing.
constexpr const wchar_t* OpenAlDriversVariable = L"ALSOFT_DRIVERS";
constexpr const wchar_t* OpenAlNullDriver = L"null";

// Stack words printed with an access violation.
constexpr int StackWordsLogged = 8;

bool playMode = false;
bool background = false;

/** True when `option` is one of the arguments. */
bool HasOption(int argc, wchar_t** argv, const wchar_t* option)
{
    for (int index = 1; index < argc; index++) {
        if (wcscmp(argv[index], option) == 0) {
            return true;
        }
    }
    return false;
}

/** Let OpenAL Soft open its null device: every sound and the intro movie play silently. */
void MuteSound()
{
    _wputenv_s(OpenAlDriversVariable, OpenAlNullDriver);
    fprintf(stderr, "Test program: sound muted (ALSOFT_DRIVERS=null)\n");
}

/**
 * Log every access violation with its module and offset before gtest turns it into a failure, so
 * a crash can be symbolized afterwards: llvm-symbolizer --obj=zombsniper_tests.exe 0x<offset+0x10000000>.
 */
LONG WINAPI LogAccessViolation(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD* record = exception->ExceptionRecord;
    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       static_cast<LPCWSTR>(record->ExceptionAddress), &module);
    wchar_t moduleName[MAX_PATH] = L"?";
    if (module != nullptr) {
        GetModuleFileNameW(module, moduleName, MAX_PATH);
    }
    const uintptr_t address = reinterpret_cast<uintptr_t>(record->ExceptionAddress);
    const uintptr_t offset = address - reinterpret_cast<uintptr_t>(module);
    const char* access = "reading";
    if (record->ExceptionInformation[0] != 0) {
        access = "writing";
    }
    fprintf(stderr, "Access violation at %08x (%ls + %08x), %s %08x\n", static_cast<unsigned int>(address), moduleName,
            static_cast<unsigned int>(offset), access, static_cast<unsigned int>(record->ExceptionInformation[1]));
    // A call into a bad address leaves its return address on top of the stack; print a few words
    // of the stack so the caller can be symbolized too.
    const auto* stack = reinterpret_cast<const uint32_t*>(exception->ContextRecord->Esp);
    for (int index = 0; index < StackWordsLogged; index++) {
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery(stack + index, &info, sizeof(info)) == 0 || info.State != MEM_COMMIT) {
            break;
        }
        fprintf(stderr, "  stack[%d] %08x\n", index, stack[index]);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/** Prints each finished test into the log too (the console may not exist). */
class LogListener : public testing::EmptyTestEventListener {
    void OnTestEnd(const testing::TestInfo& info) override
    {
        const char* outcome = "PASS";
        if (info.result()->Failed()) {
            outcome = "FAIL";
        } else if (info.result()->Skipped()) {
            outcome = "SKIP";
        }
        fprintf(stderr, "[%s] %s.%s (%lld ms)\n", outcome, info.test_suite_name(), info.name(),
                static_cast<long long>(info.result()->elapsed_time()));
    }
};

} // namespace

void PrepareTestProcess(int argc, wchar_t** argv)
{
    playMode = HasOption(argc, argv, PlayOption);
    background = !playMode || SDL_getenv(HiddenVariable) != nullptr;
    if (background || SDL_getenv(MuteVariable) != nullptr) {
        MuteSound();
    }
    // Run the same tests on the other backend without editing zombsniper.cfg.
    const char* engine = SDL_getenv(EngineVariable);
    if (engine != nullptr) {
        ckz::config.engine = engine;
        fprintf(stderr, "Test program: Engine=%s from %s\n", engine, EngineVariable);
    }
}

bool RunInBackground()
{
    return background;
}

int RunTestProgram(int argc, wchar_t** argv)
{
    const char* m3gTestFolder = SDL_getenv(ckz::m3g::LoadTestVariable);
    if (m3gTestFolder != nullptr) {
        // Native M3G engine check (PORT_PLAN.md, P2 / M0) instead of the game.
        return ckz::m3g::RunLoadTest(m3gTestFolder);
    }
    if (playMode) {
        return ckz::RunApplet();
    }
    AddVectoredExceptionHandler(1, LogAccessViolation);
    testing::InitGoogleTest(&argc, argv);
    testing::UnitTest::GetInstance()->listeners().Append(new LogListener);
    const int result = RUN_ALL_TESTS();
    fprintf(stderr, "Test program: %d failed test(s)\n", testing::UnitTest::GetInstance()->failed_test_count());
    // End the process here instead of shutting the game down: stopping it in the middle of a
    // mission or menu is not a path the tests check, and a crash there would hide the result.
    fflush(stdout);
    fflush(stderr);
    _exit(result);
}

} // namespace ckz::test
