// Log file and debug console, see Log.h.
// stdout and stderr are reopened on the log file, unbuffered, so every line is on disk at once and
// survives a crash. The console is fed by a thread that copies what the log file gains.
#include "Log.h"
#include <windows.h>
#include <stdio.h>

namespace ckz {
namespace {

// How often the console thread looks for new log text.
constexpr DWORD ConsolePollMs = 50;

std::filesystem::path logPath;
HANDLE logReader = INVALID_HANDLE_VALUE;
HANDLE consoleWriter = INVALID_HANDLE_VALUE;
HANDLE consoleThread = nullptr;
volatile bool consoleRunning = false;

/** Copy everything the log file gained since the last call to the console. */
void CopyNewLogText()
{
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(logReader, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        DWORD written = 0;
        WriteFile(consoleWriter, buffer, read, &written, nullptr);
    }
}

DWORD WINAPI FollowLog(void*)
{
    while (consoleRunning) {
        CopyNewLogText();
        Sleep(ConsolePollMs);
    }
    return 0;
}

} // namespace

std::filesystem::path ExecutableFolder()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

void StartLog()
{
    wchar_t executable[MAX_PATH];
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    logPath = std::filesystem::path(executable).replace_extension(L".log");
    // Empty the file, then append from both streams so their lines stay in order.
    FILE* file = _wfopen(logPath.c_str(), L"w");
    if (file != nullptr) {
        fclose(file);
    }
    _wfreopen(logPath.c_str(), L"a", stdout);
    _wfreopen(logPath.c_str(), L"a", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    SYSTEMTIME now;
    GetLocalTime(&now);
    fprintf(stderr, "Log started %04d-%02d-%02d %02d:%02d:%02d\n", now.wYear, now.wMonth, now.wDay, now.wHour,
            now.wMinute, now.wSecond);
}

void OpenLogConsole()
{
    if (!AllocConsole()) {
        fprintf(stderr, "Could not open the debug console (Win32 error %lu)\n", GetLastError());
        return;
    }
    // The log holds UTF-8 file names.
    SetConsoleOutputCP(CP_UTF8);
    consoleWriter = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, 0, nullptr);
    logReader = CreateFileW(logPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, 0, nullptr);
    if (consoleWriter == INVALID_HANDLE_VALUE || logReader == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Could not connect the debug console to the log (Win32 error %lu)\n", GetLastError());
        return;
    }
    consoleRunning = true;
    consoleThread = CreateThread(nullptr, 0, FollowLog, nullptr, 0, nullptr);
}

void StopLogConsole()
{
    if (consoleThread == nullptr) {
        return;
    }
    consoleRunning = false;
    WaitForSingleObject(consoleThread, INFINITE);
    CloseHandle(consoleThread);
    consoleThread = nullptr;
    CopyNewLogText();
}

} // namespace ckz
