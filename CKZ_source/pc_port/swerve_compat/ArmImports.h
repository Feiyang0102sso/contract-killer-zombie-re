#pragma once
#include <stdint.h>

namespace ckz::arm {

using NativeImport = uint64_t (*)(const uint32_t*);

NativeImport FindImport(const char* name);

} // namespace ckz::arm
