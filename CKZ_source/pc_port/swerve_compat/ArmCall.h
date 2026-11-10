#pragma once

#include <stdint.h>
#include <stddef.h>
#include <type_traits>

namespace ckz::arm {

uint32_t Call(uint32_t entry, const uint32_t* arguments, size_t count);
bool IsCode(uint32_t address);
const void* ResolvePointer(uint32_t address);
uint32_t EncodePointer(const void* address);
/** Start or stop the guest hot-spot profiler (needs CKZ_ARM_PROFILE); stopping writes arm_profile.txt. */
void ToggleProfile();

// The original ARMv7 calling convention passes these values as 32-bit words.
template<class T>
uint32_t Pack(T value)
{
    static_assert(sizeof(T) <= sizeof(uint32_t));
    if constexpr (std::is_pointer_v<T>) {
        return EncodePointer(reinterpret_cast<const void*>(value));
    } else if constexpr (std::is_same_v<T, float>) {
        union { float real; uint32_t word; } packed;
        packed.real = value;
        return packed.word;
    } else {
        return static_cast<uint32_t>(value);
    }
}

template<class Result, class... Arguments>
Result Invoke(uint32_t entry, Arguments... arguments)
{
    const uint32_t words[] = { 0, Pack(arguments)... };
    const uint32_t result = Call(entry, words + 1, sizeof...(arguments));
    if constexpr (std::is_pointer_v<Result>) {
        return reinterpret_cast<Result>(const_cast<void*>(ResolvePointer(result)));
    } else if constexpr (!std::is_void_v<Result>) {
        return static_cast<Result>(result);
    }
}

template<class Result, class... Arguments>
Result Method(const void* object, size_t slot, Arguments... arguments)
{
    const uint32_t tableAddress = *static_cast<const uint32_t*>(object);
    const auto* table = static_cast<const uint32_t*>(ResolvePointer(tableAddress));
    return Invoke<Result>(table[slot], object, arguments...);
}

template<class Signature>
struct Function;

template<class Result, class... Arguments>
struct Function<Result(Arguments...)> {
    uint32_t entry;

    Result operator()(Arguments... arguments) const
    {
        if (IsCode(entry)) {
            return Invoke<Result>(entry, arguments...);
        }
        return reinterpret_cast<Result (*)(Arguments...)>(entry)(arguments...);
    }
};

template<class T>
struct Table {
    uint32_t address;

    const T* operator->() const
    {
        return static_cast<const T*>(ResolvePointer(address));
    }
};

} // namespace ckz::arm
