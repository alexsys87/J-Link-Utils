// TypedValue.h
// One place for data types and byte order: every widget converts between raw
// target bytes and typed values with these functions, so LittleEndian means the
// same everywhere. Include after the data type constants and the DataReadWrite*
// declarations (widget_common.h includes it).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace typed_value {

// Size in bytes of a value of the given type (unknown types are treated as 32-bit).
inline int Size(int dataType) {
    switch (dataType) {
    case BOOLEAN: case S08_DEC: case U08_DEC: return 1;
    case S16_DEC: case U16_DEC: return 2;
    case S64_DEC: case U64_DEC: return 8;
    default: return 4;
    }
}

inline uint64_t SwapBytes(uint64_t raw, int size) {
    uint64_t out = 0;
    for (int i = 0; i < size; ++i) out |= ((raw >> (8 * i)) & 0xFF) << (8 * (size - 1 - i));
    return out;
}

// FLOAT_INV is a float stored in the byte order opposite to littleEndian.
inline bool NeedsSwap(int dataType, bool littleEndian) {
    return (dataType == FLOAT_INV) ? littleEndian : !littleEndian;
}

// raw holds the target bytes in memory order, first byte in the lowest 8 bits.
inline double Decode(uint64_t raw, int dataType, bool littleEndian) {
    const int size = Size(dataType);
    if (size < 8) raw &= (UINT64_C(1) << (8 * size)) - 1;
    if (size > 1 && NeedsSwap(dataType, littleEndian)) raw = SwapBytes(raw, size);
    switch (dataType) {
    case BOOLEAN: return (raw & 1) ? 1.0 : 0.0;
    case S08_DEC: return (double)(int8_t)raw;
    case U08_DEC: return (double)(uint8_t)raw;
    case S16_DEC: return (double)(int16_t)raw;
    case U16_DEC: return (double)(uint16_t)raw;
    case S32_DEC: return (double)(int32_t)raw;
    case U32_DEC: return (double)(uint32_t)raw;
    case S64_DEC: return (double)(int64_t)raw;
    case U64_DEC: return (double)raw;
    case FLOAT: case FLOAT_INV: {
        const uint32_t bits = (uint32_t)raw;
        float f;
        memcpy(&f, &bits, sizeof(f));
        return f;
    }
    default: return 0.0;
    }
}

// Integers are rounded to the nearest value and clamped to the range of the type.
inline uint64_t Encode(double value, int dataType, bool littleEndian) {
    auto clampRound = [value](double lo, double hi) {
        if (std::isnan(value)) return 0.0;
        const double r = std::round(value);
        return r < lo ? lo : (r > hi ? hi : r);
    };
    uint64_t raw = 0;
    switch (dataType) {
    case BOOLEAN: raw = (value != 0.0) ? 1 : 0; break;
    case S08_DEC: raw = (uint8_t)(int8_t)clampRound(-128.0, 127.0); break;
    case U08_DEC: raw = (uint8_t)clampRound(0.0, 255.0); break;
    case S16_DEC: raw = (uint16_t)(int16_t)clampRound(-32768.0, 32767.0); break;
    case U16_DEC: raw = (uint16_t)clampRound(0.0, 65535.0); break;
    case S32_DEC: raw = (uint32_t)(int32_t)clampRound(-2147483648.0, 2147483647.0); break;
    case U32_DEC: raw = (uint32_t)clampRound(0.0, 4294967295.0); break;
    case S64_DEC: raw = (uint64_t)(int64_t)clampRound(-9223372036854775808.0, 9223372036854774784.0); break;
    case U64_DEC: raw = (uint64_t)clampRound(0.0, 18446744073709549568.0); break;
    case FLOAT: case FLOAT_INV: {
        const float f = (float)value;
        uint32_t bits;
        memcpy(&bits, &f, sizeof(bits));
        raw = bits;
        break;
    }
    default: break;
    }
    const int size = Size(dataType);
    if (size > 1 && NeedsSwap(dataType, littleEndian)) raw = SwapBytes(raw, size);
    return raw;
}

// Decodes a value stored at bytes (memory order).
inline double DecodeBytes(const uint8_t* bytes, int dataType, bool littleEndian) {
    uint64_t raw = 0;
    for (int i = 0; i < Size(dataType); ++i) raw |= (uint64_t)bytes[i] << (8 * i);
    return Decode(raw, dataType, littleEndian);
}

} // namespace typed_value

// Reads a typed value from the cache (the range stays polled while it is read).
inline double ReadTypedValue(MainContext* ctx, uint32_t addr, int dataType, bool littleEndian) {
    uint64_t raw;
    switch (typed_value::Size(dataType)) {
    case 1: raw = DataReadWriteU08(ctx, nullptr, nullptr, &addr); break;
    case 2: raw = DataReadWriteU16(ctx, nullptr, nullptr, &addr); break;
    case 8: raw = DataReadWriteU64(ctx, nullptr, nullptr, &addr); break;
    default: raw = DataReadWriteU32(ctx, nullptr, nullptr, &addr); break;
    }
    return typed_value::Decode(raw, dataType, littleEndian);
}

// Writes a typed value to the target.
inline void WriteTypedValue(MainContext* ctx, uint32_t addr, int dataType, bool littleEndian, double value) {
    const uint64_t raw = typed_value::Encode(value, dataType, littleEndian);
    switch (typed_value::Size(dataType)) {
    case 1: { uint8_t v = (uint8_t)raw; DataReadWriteU08(ctx, nullptr, &v, &addr); break; }
    case 2: { uint16_t v = (uint16_t)raw; DataReadWriteU16(ctx, nullptr, &v, &addr); break; }
    case 8: { uint64_t v = raw; DataReadWriteU64(ctx, nullptr, &v, &addr); break; }
    default: { uint32_t v = (uint32_t)raw; DataReadWriteU32(ctx, nullptr, &v, &addr); break; }
    }
}
