#pragma once
#include <cstddef>
#include <cstdint>

namespace ui_safety {
inline bool IsMappedRange(uint32_t address, uint32_t base, std::size_t bytes,
                          std::size_t capacity) {
    if (address < base) return false;
    const uint64_t offset = static_cast<uint64_t>(address) - base;
    if (offset > capacity || bytes > capacity - static_cast<std::size_t>(offset)) return false;
    // The physical range must also fit in the 32-bit address space.
    return bytes <= (uint64_t{1} << 32) - address;
}
} // namespace ui_safety
