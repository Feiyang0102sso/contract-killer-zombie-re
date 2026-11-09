// Run only the original ARM Swerve core. Game and platform code run as native x86.
#include "ArmCall.h"
#include "ArmImports.h"
#include <SwerveApi.h>
#include "../platform_sdl/Host.h"
#include <windows.h>
#include <unicorn/unicorn.h>
#include <unicorn/arm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>
#include <map>
#include <vector>

namespace ckz::arm {
namespace {

// High guest addresses leave Win32 host stack, heap and DLL mappings independent.
constexpr uint32_t GuestOffset = 0xe0000000;
constexpr uint32_t ImageBase = GuestOffset + 0x360000;
constexpr uint32_t CodeStart = GuestOffset + 0x364814;
constexpr uint32_t CodeEnd = GuestOffset + 0x3c8dc0;
constexpr uint32_t SwvNew = GuestOffset + 0x3ba444;
constexpr uint32_t TrapBase = GuestOffset + 0x510000;
constexpr size_t ImageSize = TrapBase - ImageBase;
constexpr size_t PageSize = 0x1000;
constexpr size_t StackSize = 0x200000;
constexpr size_t ArgumentWords = 12;

const char* const ImportNames[] = {
    "",
#include "ArmImportNames.inc"
    "malij297_PopulateVBO_Contiguous",
};
constexpr size_t ImportCount = sizeof(ImportNames) / sizeof(ImportNames[0]);

enum class ImportKind { Native, SetJump, LongJump, Environment };

struct Import {
    ImportKind kind;
    NativeImport call;
};

struct SavedJump {
    uint32_t buffer;
    uint32_t resume;
    uc_context* context;
};

struct Frame {
    Frame* parent;
    std::vector<SavedJump> jumps;
    uc_context* parentContext;
    uint32_t resume;
};

uc_engine* engine;
Frame* activeFrame;
Frame* jumpTarget;
SavedJump* pendingJump;
uint32_t jumpValue;
uint32_t stackTop;
uint32_t reachedTrap;
unsigned char* image;
Import imports[ImportCount];
std::map<uint64_t, uint64_t> mappedRanges;

void Check(uc_err status, const char* operation)
{
    if (status != UC_ERR_OK) {
        fprintf(stderr, "Swerve ARM %s: %s\n", operation, uc_strerror(status));
        abort();
    }
}

uint32_t Read(int reg)
{
    uint32_t value;
    Check(uc_reg_read(engine, reg, &value), "read register");
    return value;
}

void Write(int reg, uint32_t value)
{
    Check(uc_reg_write(engine, reg, &value), "write register");
}

uint32_t HostAddress(const void* pointer)
{
    return reinterpret_cast<uint32_t>(pointer);
}

uc_context* SaveContext()
{
    uc_context* context;
    Check(uc_context_alloc(engine, &context), "allocate context");
    Check(uc_context_save(engine, context), "save context");
    return context;
}

// Map each committed Win32 region in one transaction. Rebuilding Unicorn's
// memory topology for every heap page makes large M3G files very slow to load.
bool MapHostPage(uc_engine*, uc_mem_type access, uint64_t address, int, int64_t, void*)
{
    const uint64_t page = address & ~(static_cast<uint64_t>(PageSize) - 1);
    if (access != UC_MEM_FETCH_UNMAPPED && page >= 0x10000 && page <= UINT32_MAX) {
        auto* host = reinterpret_cast<void*>(static_cast<uint32_t>(page));
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery(host, &info, sizeof(info)) != 0 && info.State == MEM_COMMIT &&
            !(info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            uint32_t permissions = UC_PROT_READ;
            if (info.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
                permissions |= UC_PROT_WRITE;
            }
            uint64_t first = HostAddress(info.BaseAddress);
            uint64_t last = first + info.RegionSize;
            const auto next = mappedRanges.upper_bound(page);
            if (next != mappedRanges.end() && next->first < last) {
                last = next->first;
            }
            if (next != mappedRanges.begin()) {
                const auto previous = std::prev(next);
                if (previous->second > first) {
                    first = previous->second;
                }
            }
            if (first <= page && page < last &&
                uc_mem_map_ptr(engine, first, static_cast<size_t>(last - first), permissions,
                               reinterpret_cast<void*>(static_cast<uint32_t>(first))) == UC_ERR_OK) {
                mappedRanges.emplace(first, last);
                return true;
            }
        }
    }
    fprintf(stderr, "Swerve unmapped access %d at %08llx, PC %08x LR %08x\n",
            access, address, Read(UC_ARM_REG_PC), Read(UC_ARM_REG_LR));
    return false;
}

void Trap(uc_engine*, uint64_t address, uint32_t, void*)
{
    reachedTrap = static_cast<uint32_t>(address);
    uc_emu_stop(engine);
}

bool Initialize()
{
    image = static_cast<unsigned char*>(VirtualAlloc(nullptr, ImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    auto* stack = static_cast<unsigned char*>(VirtualAlloc(nullptr, StackSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (image == nullptr || stack == nullptr) {
        fprintf(stderr, "Could not allocate Swerve memory (Win32 error %lu)\n", GetLastError());
        return false;
    }

    const auto path = std::filesystem::u8path(SDL_GetBasePath()) / SwerveImageName;
    FILE* file = _wfopen(path.c_str(), L"rb");
    if (file == nullptr) {
        fwprintf(stderr, L"Swerve ARM image missing: %ls\n", path.c_str());
        return false;
    }
    char magic[8];
    const bool complete = fread(magic, 1, sizeof(magic), file) == sizeof(magic) &&
        memcmp(magic, "CKZARM2", sizeof(magic)) == 0 &&
        fread(image, 1, ImageSize, file) == ImageSize && fgetc(file) == EOF;
    fclose(file);
    if (!complete) {
        fprintf(stderr, "Invalid Swerve ARM image size\n");
        return false;
    }

    for (size_t index = 1; index < ImportCount; index++) {
        const char* name = ImportNames[index];
        Import& entry = imports[index];
        entry.kind = ImportKind::Native;
        entry.call = FindImport(name);
        if (strcmp(name, "setjmp") == 0) {
            entry.kind = ImportKind::SetJump;
        } else if (strcmp(name, "longjmp") == 0) {
            entry.kind = ImportKind::LongJump;
        } else if (strcmp(name, "malij297_GetEnvironment") == 0) {
            entry.kind = ImportKind::Environment;
        } else if (entry.call == nullptr) {
            fprintf(stderr, "Unknown Swerve import %s\n", name);
            return false;
        }
    }

    Check(uc_open(UC_ARCH_ARM, UC_MODE_ARM, &engine), "open");
    Check(uc_mem_map_ptr(engine, ImageBase, ImageSize, UC_PROT_ALL, image), "map image");
    Check(uc_mem_map_ptr(engine, HostAddress(stack), StackSize, UC_PROT_READ | UC_PROT_WRITE, stack), "map stack");
    Check(uc_mem_map(engine, TrapBase, PageSize, UC_PROT_ALL), "map imports");
    stackTop = HostAddress(stack + StackSize);
    mappedRanges.emplace(ImageBase, TrapBase + PageSize);
    mappedRanges.emplace(HostAddress(stack), stackTop);
    uint32_t trapInstructions[PageSize / sizeof(uint32_t)];
    for (uint32_t& instruction : trapInstructions) {
        instruction = 0xe12fff1e; // bx lr; trap dispatch normally stops before this executes.
    }
    Check(uc_mem_write(engine, TrapBase, trapInstructions, sizeof(trapInstructions)), "write import traps");
    uc_hook hook;
    Check(uc_hook_add(engine, &hook, UC_HOOK_CODE, reinterpret_cast<void*>(Trap), nullptr,
                      TrapBase + 4, TrapBase + PageSize - 1), "hook imports");
    Check(uc_hook_add(engine, &hook, UC_HOOK_MEM_UNMAPPED, reinterpret_cast<void*>(MapHostPage), nullptr,
                      1, 0), "hook shared memory");
    Write(UC_ARM_REG_C1_C0_2, 0x00f00000);
    Write(UC_ARM_REG_FPEXC, 0x40000000);
    fprintf(stderr, "Swerve ARM initialized (%zu imports)\n", ImportCount - 1);
    return true;
}

void SetJump(Frame& frame, uint32_t buffer, uint32_t resume)
{
    for (SavedJump& jump : frame.jumps) {
        if (jump.buffer == buffer) {
            Check(uc_context_save(engine, jump.context), "refresh setjmp");
            jump.resume = resume;
            return;
        }
    }
    frame.jumps.push_back({ buffer, resume, SaveContext() });
}

void LongJump(Frame& frame, uint32_t buffer, uint32_t value)
{
    for (Frame* caller = &frame; caller != nullptr; caller = caller->parent) {
        for (SavedJump& jump : caller->jumps) {
            if (jump.buffer == buffer) {
                jumpTarget = caller;
                pendingJump = &jump;
                jumpValue = value;
                if (jumpValue == 0) {
                    jumpValue = 1;
                }
                return;
            }
        }
    }
    fprintf(stderr, "Swerve longjmp without matching setjmp: %08x\n", buffer);
    abort();
}

uint64_t Dispatch(Frame& frame, uint32_t address)
{
    const size_t index = (address - TrapBase) / 4;
    if (index == 0 || index >= ImportCount) {
        fprintf(stderr, "Invalid Swerve import address %08x\n", address);
        abort();
    }

    uint32_t arguments[ArgumentWords];
    arguments[0] = Read(UC_ARM_REG_R0);
    arguments[1] = Read(UC_ARM_REG_R1);
    arguments[2] = Read(UC_ARM_REG_R2);
    arguments[3] = Read(UC_ARM_REG_R3);
    const auto* stack = reinterpret_cast<const uint32_t*>(Read(UC_ARM_REG_SP));
    memcpy(arguments + 4, stack, (ArgumentWords - 4) * sizeof(uint32_t));

    const Import& entry = imports[index];
    switch (entry.kind) {
    case ImportKind::SetJump:
        SetJump(frame, arguments[0], frame.resume);
        return 0;
    case ImportKind::LongJump:
        LongJump(frame, arguments[0], arguments[1]);
        return 0;
    case ImportKind::Environment: {
        const uint64_t result = entry.call(arguments);
        auto* environment = reinterpret_cast<malij297_Environment*>(arguments[0]);
        const uint32_t populateVbo = TrapBase + (ImportCount - 1) * 4;
        environment->m_populateVBO = reinterpret_cast<decltype(environment->m_populateVBO)>(populateVbo);
        return result;
    }
    case ImportKind::Native:
        return entry.call(arguments);
    }
    abort();
}

} // namespace

bool IsCode(uint32_t address)
{
    return address >= CodeStart && address < CodeEnd;
}

const void* ResolvePointer(uint32_t address)
{
    if (address >= ImageBase && address < TrapBase) {
        return image + address - ImageBase;
    }
    return reinterpret_cast<const void*>(address);
}

uint32_t EncodePointer(const void* address)
{
    const uint32_t native = HostAddress(address);
    const uint32_t imageStart = HostAddress(image);
    if (image != nullptr && native - imageStart < ImageSize) {
        return ImageBase + native - imageStart;
    }
    return native;
}

uint32_t Call(uint32_t entry, const uint32_t* arguments, size_t count)
{
    if (!IsCode(entry) || count > ArgumentWords) {
        fprintf(stderr, "Invalid ARM Swerve call %08x (%zu words)\n", entry, count);
        abort();
    }
    if (jumpTarget != nullptr) {
        return 0;
    }

    Frame frame = { activeFrame, {}, nullptr, entry };
    uint32_t stackPointer = stackTop;
    if (activeFrame != nullptr) {
        frame.parentContext = SaveContext();
        stackPointer = Read(UC_ARM_REG_SP);
    }
    activeFrame = &frame;
    stackPointer = (stackPointer - 128) & ~7u;
    Write(UC_ARM_REG_SP, stackPointer);
    Write(UC_ARM_REG_LR, TrapBase);
    const int registers[] = { UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3 };
    for (size_t index = 0; index < count && index < 4; index++) {
        Write(registers[index], arguments[index]);
    }
    if (count > 4) {
        memcpy(reinterpret_cast<void*>(stackPointer), arguments + 4, (count - 4) * sizeof(uint32_t));
    }

    while (true) {
        reachedTrap = 0;
        const uc_err status = uc_emu_start(engine, frame.resume, TrapBase, 0, 0);
        if (status != UC_ERR_OK) {
            fprintf(stderr, "Swerve call entry %08x, PC %08x LR %08x\n",
                    entry, Read(UC_ARM_REG_PC), Read(UC_ARM_REG_LR));
            Check(status, "execute");
        }
        if (reachedTrap == 0) {
            break;
        }

        frame.resume = Read(UC_ARM_REG_LR);
        const uint64_t result = Dispatch(frame, reachedTrap);
        if (jumpTarget == &frame) {
            Check(uc_context_restore(engine, pendingJump->context), "restore longjmp");
            frame.resume = pendingJump->resume;
            Write(UC_ARM_REG_R0, jumpValue);
            jumpTarget = nullptr;
            pendingJump = nullptr;
        } else if (jumpTarget != nullptr) {
            break;
        } else {
            Write(UC_ARM_REG_R0, static_cast<uint32_t>(result));
            Write(UC_ARM_REG_R1, static_cast<uint32_t>(result >> 32));
        }
    }

    uint32_t result = Read(UC_ARM_REG_R0);
    if (jumpTarget != nullptr) {
        result = 0;
    }
    for (const SavedJump& jump : frame.jumps) {
        uc_context_free(jump.context);
    }
    activeFrame = frame.parent;
    if (frame.parentContext != nullptr) {
        Check(uc_context_restore(engine, frame.parentContext), "restore caller");
        uc_context_free(frame.parentContext);
    }
    return result;
}

} // namespace ckz::arm

extern "C" int swvNew(ISwerve** swerve)
{
    if (ckz::arm::engine == nullptr && !ckz::arm::Initialize()) {
        return 1;
    }
    return ckz::arm::Invoke<int>(ckz::arm::SwvNew, swerve);
}
