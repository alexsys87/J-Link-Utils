#pragma once

#include "WriteProtection.h"

#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <system_error>

inline unsigned WriteValueSize(WriteValueType type)
{
    switch (type) {
    case WriteValueType::U8: case WriteValueType::I8: return 1;
    case WriteValueType::U16: case WriteValueType::I16: return 2;
    case WriteValueType::U32: case WriteValueType::I32:
    case WriteValueType::Float32: return 4;
    case WriteValueType::U64: case WriteValueType::I64:
    case WriteValueType::Float64: return 8;
    }
    return 0;
}

namespace WriteProtectionRulesDetail {

struct Bounds {
    uint64_t unsignedMinimum = 0, unsignedMaximum = 0;
    int64_t signedMinimum = 0, signedMaximum = 0;
    double floatMinimum = 0, floatMaximum = 0;
};

inline bool Fail(std::string& error, const char* message)
{
    error = message;
    return false;
}

template <typename T>
inline bool ParseInteger(const std::string& text, T& value)
{
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

template <typename T>
inline bool ParseFloat(const std::string& text, T& value)
{
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value,
                                       std::chars_format::general);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size()
        && std::isfinite(value);
}

inline bool ParseRule(const WriteValueRule& rule, Bounds& bounds, std::string& error)
{
    const unsigned width = WriteValueSize(rule.type);
    if (!width) return Fail(error, "Invalid value rule type.");
    if (!rule.address) return Fail(error, "Value rule address must be nonzero.");
    if (uint64_t(rule.address) + width > (uint64_t{1} << 32))
        return Fail(error, "Value rule extends beyond the 32-bit address space.");

    switch (rule.type) {
    case WriteValueType::U8: case WriteValueType::U16:
    case WriteValueType::U32: case WriteValueType::U64: {
        if (!ParseInteger(rule.minimum, bounds.unsignedMinimum)
            || !ParseInteger(rule.maximum, bounds.unsignedMaximum))
            return Fail(error, "Unsigned bounds must be exact decimal integers.");
        const uint64_t maximum = width == 8 ? UINT64_MAX : (uint64_t{1} << (width * 8)) - 1;
        if (bounds.unsignedMinimum > maximum || bounds.unsignedMaximum > maximum)
            return Fail(error, "Unsigned bounds exceed the selected type.");
        if (bounds.unsignedMinimum > bounds.unsignedMaximum)
            return Fail(error, "Value rule minimum exceeds maximum.");
        break;
    }
    case WriteValueType::I8: case WriteValueType::I16:
    case WriteValueType::I32: case WriteValueType::I64: {
        if (!ParseInteger(rule.minimum, bounds.signedMinimum)
            || !ParseInteger(rule.maximum, bounds.signedMaximum))
            return Fail(error, "Signed bounds must be exact decimal integers.");
        const int64_t minimum = width == 8 ? INT64_MIN : -(int64_t{1} << (width * 8 - 1));
        const int64_t maximum = width == 8 ? INT64_MAX : (int64_t{1} << (width * 8 - 1)) - 1;
        if (bounds.signedMinimum < minimum || bounds.signedMinimum > maximum
            || bounds.signedMaximum < minimum || bounds.signedMaximum > maximum)
            return Fail(error, "Signed bounds exceed the selected type.");
        if (bounds.signedMinimum > bounds.signedMaximum)
            return Fail(error, "Value rule minimum exceeds maximum.");
        break;
    }
    case WriteValueType::Float32: {
        float minimum = 0, maximum = 0;
        if (!ParseFloat(rule.minimum, minimum) || !ParseFloat(rule.maximum, maximum))
            return Fail(error, "Float32 bounds must be finite, representable numbers.");
        bounds.floatMinimum = minimum;
        bounds.floatMaximum = maximum;
        if (minimum > maximum) return Fail(error, "Value rule minimum exceeds maximum.");
        break;
    }
    case WriteValueType::Float64:
        if (!ParseFloat(rule.minimum, bounds.floatMinimum)
            || !ParseFloat(rule.maximum, bounds.floatMaximum))
            return Fail(error, "Float64 bounds must be finite, representable numbers.");
        if (bounds.floatMinimum > bounds.floatMaximum)
            return Fail(error, "Value rule minimum exceeds maximum.");
        break;
    }
    return true;
}

// Assemble the target's little-endian bytes independently of host byte order.
inline uint64_t DecodeUnsigned(const uint8_t* bytes, unsigned width)
{
    uint64_t value = 0;
    for (unsigned i = 0; i < width; ++i) value |= uint64_t(bytes[i]) << (8 * i);
    return value;
}

inline bool CheckValue(const WriteValueRule& rule, const Bounds& bounds,
                       const uint8_t* bytes, std::string& error)
{
    const unsigned width = WriteValueSize(rule.type);
    const uint64_t raw = DecodeUnsigned(bytes, width);
    bool withinBounds = false;
    switch (rule.type) {
    case WriteValueType::U8: case WriteValueType::U16:
    case WriteValueType::U32: case WriteValueType::U64:
        withinBounds = raw >= bounds.unsignedMinimum && raw <= bounds.unsignedMaximum;
        break;
    case WriteValueType::I8: case WriteValueType::I16:
    case WriteValueType::I32: case WriteValueType::I64: {
        const uint64_t sign = uint64_t{1} << (width * 8 - 1);
        const uint64_t mask = width == 8 ? UINT64_MAX : (uint64_t{1} << (width * 8)) - 1;
        // The complement fits int64_t even for INT64_MIN. Neither conversion
        // of an out-of-range unsigned value nor negation of INT64_MIN is used.
        const int64_t value = (raw & sign) ? -static_cast<int64_t>((~raw) & mask) - 1
                                          : static_cast<int64_t>(raw);
        withinBounds = value >= bounds.signedMinimum && value <= bounds.signedMaximum;
        break;
    }
    case WriteValueType::Float32: {
        static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                      "Float32 rules require IEEE-754 binary32.");
        const uint32_t bits = static_cast<uint32_t>(raw);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        withinBounds = std::isfinite(value)
            && value >= bounds.floatMinimum && value <= bounds.floatMaximum;
        break;
    }
    case WriteValueType::Float64: {
        static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
                      "Float64 rules require IEEE-754 binary64.");
        double value;
        std::memcpy(&value, &raw, sizeof(value));
        withinBounds = std::isfinite(value)
            && value >= bounds.floatMinimum && value <= bounds.floatMaximum;
        break;
    }
    }
    return withinBounds || Fail(error, "Write value is non-finite or outside the configured bounds.");
}

} // namespace WriteProtectionRulesDetail

inline bool ValidateWriteValueRule(const WriteValueRule& rule, std::string& error)
{
    error.clear();
    WriteProtectionRulesDetail::Bounds bounds;
    return WriteProtectionRulesDetail::ParseRule(rule, bounds, error);
}

inline bool ValidateProtectedWrite(const WriteProtectionStatus& status, uint32_t address,
                                   const uint8_t* bytes, uint16_t size, std::string& error)
{
    using namespace WriteProtectionRulesDetail;
    error.clear();
    if (status.readOnly) return Fail(error, "Target writes are locked (read-only).");
    if (!size) return Fail(error, "Write size must be nonzero.");
    if (!bytes) return Fail(error, "Write data is null.");
    const uint64_t end = uint64_t(address) + size; // exclusive; never wrap at 2^32
    if (end > (uint64_t{1} << 32))
        return Fail(error, "Write extends beyond the 32-bit address space.");
    if (status.restrictAddresses) {
        if (status.firstAddress > status.lastAddress)
            return Fail(error, "Configured write address range is invalid.");
        if (address < status.firstAddress || end - 1 > status.lastAddress)
            return Fail(error, "Entire write must be within the allowed address range.");
    }
    for (const auto& rule : status.rules) {
        Bounds bounds;
        // Caller-owned configuration is checked, even for non-overlapping
        // rules, so malformed configuration cannot silently disable protection.
        if (!ParseRule(rule, bounds, error)) return false;
        const unsigned width = WriteValueSize(rule.type);
        const uint64_t ruleEnd = uint64_t(rule.address) + width;
        if (uint64_t(address) < ruleEnd && uint64_t(rule.address) < end) {
            if (address != rule.address || size != width)
                return Fail(error, "Write overlaps a typed rule without matching its exact address and width.");
            if (!CheckValue(rule, bounds, bytes, error)) return false;
        }
    }
    return true;
}
