#include "WriteProtectionRules.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;

void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition) {
        std::cerr << "Line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(bool(expression), #expression, __LINE__)

std::array<uint8_t, 8> Bytes(uint64_t value)
{
    std::array<uint8_t, 8> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = uint8_t(value >> (8 * i));
    return bytes;
}

void Rule(WriteValueType type, const std::string& minimum, const std::string& maximum,
          bool expected, uint32_t address = 0x100)
{
    std::string error = "stale error";
    CHECK(ValidateWriteValueRule({address, type, minimum, maximum}, error) == expected);
    CHECK(error.empty() == expected);
}

void Write(const WriteProtectionStatus& status, uint32_t address, uint16_t size,
           uint64_t value, bool expected)
{
    const auto bytes = Bytes(value);
    // Larger sizes are used only in tests rejected before any data read, or
    // without typed rules, for which the validator does not examine the data.
    std::string error = "stale error";
    CHECK(ValidateProtectedWrite(status, address, bytes.data(), size, error) == expected);
    CHECK(error.empty() == expected);
}

WriteProtectionStatus WithRule(WriteValueType type, const char* minimum, const char* maximum)
{
    WriteProtectionStatus status;
    status.readOnly = false;
    status.rules.push_back({0x100, type, minimum, maximum});
    return status;
}

void TestTypesAndIntegerLimits()
{
    const std::array<WriteValueType, 10> types = {{WriteValueType::U8, WriteValueType::U16,
        WriteValueType::U32, WriteValueType::U64, WriteValueType::I8, WriteValueType::I16,
        WriteValueType::I32, WriteValueType::I64, WriteValueType::Float32, WriteValueType::Float64}};
    const std::array<unsigned, 10> widths = {{1, 2, 4, 8, 1, 2, 4, 8, 4, 8}};
    for (unsigned i = 0; i < types.size(); ++i) CHECK(WriteValueSize(types[i]) == widths[i]);
    CHECK(WriteValueSize(static_cast<WriteValueType>(-1)) == 0);
    CHECK(WriteValueSize(static_cast<WriteValueType>(100)) == 0);
    Rule(static_cast<WriteValueType>(100), "0", "1", false);

    Rule(WriteValueType::U8, "0", "255", true);
    Rule(WriteValueType::U8, "0", "256", false);
    Rule(WriteValueType::U8, "256", "256", false);
    Rule(WriteValueType::U16, "0", "65535", true);
    Rule(WriteValueType::U16, "0", "65536", false);
    Rule(WriteValueType::U32, "0", "4294967295", true);
    Rule(WriteValueType::U32, "0", "4294967296", false);
    Rule(WriteValueType::U64, "0", "18446744073709551615", true);
    Rule(WriteValueType::U64, "0", "18446744073709551616", false);
    Rule(WriteValueType::U64, "9007199254740993", "9007199254740992", false);
    Rule(WriteValueType::I8, "-128", "127", true);
    Rule(WriteValueType::I8, "-129", "127", false);
    Rule(WriteValueType::I8, "-128", "128", false);
    Rule(WriteValueType::I16, "-32768", "32767", true);
    Rule(WriteValueType::I16, "-32769", "32767", false);
    Rule(WriteValueType::I16, "-32768", "32768", false);
    Rule(WriteValueType::I32, "-2147483648", "2147483647", true);
    Rule(WriteValueType::I32, "-2147483649", "2147483647", false);
    Rule(WriteValueType::I32, "-2147483648", "2147483648", false);
    Rule(WriteValueType::I64, "-9223372036854775808", "9223372036854775807", true);
    Rule(WriteValueType::I64, "-9223372036854775809", "0", false);
    Rule(WriteValueType::I64, "0", "9223372036854775808", false);
    Rule(WriteValueType::I64, "9223372036854775807", "9223372036854775806", false);
    Rule(WriteValueType::I64, "-9223372036854775807", "-9223372036854775808", false);
    Rule(WriteValueType::I8, "1", "-1", false);

    for (const std::string text : {"", " 1", "1 ", "+1", "1.0", "1e0", "0x10",
                                   "1junk", "--1", "18446744073709551616"}) {
        Rule(WriteValueType::U64, text, "18446744073709551615", false);
        Rule(WriteValueType::I64, "-9223372036854775808", text, false);
    }
    Rule(WriteValueType::U64, "-1", "1", false);
    Rule(WriteValueType::U64, "-0", "1", false);
    Rule(WriteValueType::I64, "-0", "0001", true);
    Rule(WriteValueType::U64, std::string("1\0x", 3), "2", false);
}

void TestIntegerValues()
{
    auto status = WithRule(WriteValueType::U64, "9007199254740993", "9007199254740993");
    Write(status, 0x100, 8, 9007199254740992ULL, false);
    Write(status, 0x100, 8, 9007199254740993ULL, true);
    Write(status, 0x100, 8, 9007199254740994ULL, false);
    status = WithRule(WriteValueType::U64, "18446744073709551614", "18446744073709551615");
    Write(status, 0x100, 8, UINT64_MAX - 2, false);
    Write(status, 0x100, 8, UINT64_MAX - 1, true);
    Write(status, 0x100, 8, UINT64_MAX, true);
    status = WithRule(WriteValueType::U64, "0", "0");
    Write(status, 0x100, 8, 0, true);
    Write(status, 0x100, 8, UINT64_MAX, false);

    status = WithRule(WriteValueType::I64, "-9223372036854775808", "-9223372036854775808");
    Write(status, 0x100, 8, uint64_t{1} << 63, true);
    Write(status, 0x100, 8, (uint64_t{1} << 63) + 1, false);
    status = WithRule(WriteValueType::I64, "9223372036854775807", "9223372036854775807");
    Write(status, 0x100, 8, INT64_MAX, true);
    Write(status, 0x100, 8, uint64_t(INT64_MAX) - 1, false);
    Write(status, 0x100, 8, UINT64_MAX, false);
    status = WithRule(WriteValueType::I64, "-9223372036854775808", "9223372036854775807");
    for (uint64_t value : {uint64_t{0}, uint64_t{1}, UINT64_MAX, uint64_t{1} << 63,
                           uint64_t(INT64_MAX)}) Write(status, 0x100, 8, value, true);

    const std::array<WriteValueType, 4> signedTypes = {{WriteValueType::I8, WriteValueType::I16,
        WriteValueType::I32, WriteValueType::I64}};
    for (auto type : signedTypes) {
        status = WithRule(type, "-2", "2");
        const unsigned width = WriteValueSize(type);
        Write(status, 0x100, width, UINT64_MAX - 2, false);
        Write(status, 0x100, width, UINT64_MAX - 1, true);
        Write(status, 0x100, width, UINT64_MAX, true);
        Write(status, 0x100, width, 0, true);
        Write(status, 0x100, width, 2, true);
        Write(status, 0x100, width, 3, false);
        Write(status, 0x100, width, uint64_t{1} << (width * 8 - 1), false);
    }
    const std::array<WriteValueType, 4> unsignedTypes = {{WriteValueType::U8, WriteValueType::U16,
        WriteValueType::U32, WriteValueType::U64}};
    for (auto type : unsignedTypes) {
        status = WithRule(type, "1", "2");
        Write(status, 0x100, WriteValueSize(type), 0, false);
        Write(status, 0x100, WriteValueSize(type), 1, true);
        Write(status, 0x100, WriteValueSize(type), 2, true);
        Write(status, 0x100, WriteValueSize(type), 3, false);
        Write(status, 0x100, WriteValueSize(type), UINT64_MAX, false);
    }
    status = WithRule(WriteValueType::U32, "305419896", "305419896");
    Write(status, 0x100, 4, 0x12345678, true); // bytes 78 56 34 12
    Write(status, 0x100, 4, 0x78563412, false);

    status = WithRule(WriteValueType::I8, "-128", "127");
    Write(status, 0x100, 1, 0x80, true);
    Write(status, 0x100, 1, 0x7f, true);
    status = WithRule(WriteValueType::I16, "-32768", "32767");
    Write(status, 0x100, 2, 0x8000, true);
    Write(status, 0x100, 2, 0x7fff, true);
    status = WithRule(WriteValueType::I32, "-2147483648", "2147483647");
    Write(status, 0x100, 4, 0x80000000, true);
    Write(status, 0x100, 4, 0x7fffffff, true);
    status = WithRule(WriteValueType::I8, "-100", "100");
    for (unsigned raw = 0; raw < 256; ++raw) {
        const int signedValue = raw < 128 ? int(raw) : int(raw) - 256;
        Write(status, 0x100, 1, raw, signedValue >= -100 && signedValue <= 100);
    }
}

void TestFloats()
{
    for (auto type : {WriteValueType::Float32, WriteValueType::Float64}) {
        Rule(type, "-1.25", "2.5e1", true);
        Rule(type, "-0", "0", true);
        Rule(type, "2", "1", false);
        for (const std::string text : {"", "nan", "NaN", "nan(payload)", "NAN(0x123)",
                                       "inf", "-inf", "infinity", "-Infinity", "+inf",
                                       " 1", "1 ", "+1", "1x", "0x1p0", "1e9999", "1e-9999"}) {
            Rule(type, text, "2", false);
            Rule(type, "-2", text, false);
        }
        Rule(type, std::string("1\0x", 3), "2", false);
    }
    Rule(WriteValueType::Float32, "-3.4028234663852886e38", "3.4028234663852886e38", true);
    Rule(WriteValueType::Float32, "0", "3.5e38", false);
    Rule(WriteValueType::Float64, "-1.7976931348623157e308", "1.7976931348623157e308", true);
    Rule(WriteValueType::Float64, "0", "1.8e308", false);
    Rule(WriteValueType::Float32, "1.401298464324817e-45", "1", true);
    Rule(WriteValueType::Float64, "4.9406564584124654e-324", "1", true);

    auto status = WithRule(WriteValueType::Float32, "-1", "1");
    for (uint64_t bits : {0x00000000ULL, 0x80000000ULL, 0x3f800000ULL, 0xbf800000ULL,
                          0x00000001ULL, 0x80000001ULL}) Write(status, 0x100, 4, bits, true);
    for (uint64_t bits : {0x3f800001ULL, 0xbf800001ULL, 0x7f800000ULL, 0xff800000ULL,
                          0x7fc00000ULL, 0x7f800001ULL, 0xffc01234ULL})
        Write(status, 0x100, 4, bits, false);
    status = WithRule(WriteValueType::Float32, "1.5", "1.5");
    Write(status, 0x100, 4, 0x3fc00000, true);
    Write(status, 0x100, 4, 0x3fc00001, false);

    status = WithRule(WriteValueType::Float64, "-1", "1");
    for (uint64_t bits : {0x0000000000000000ULL, 0x8000000000000000ULL,
                          0x3ff0000000000000ULL, 0xbff0000000000000ULL,
                          0x0000000000000001ULL, 0x8000000000000001ULL})
        Write(status, 0x100, 8, bits, true);
    for (uint64_t bits : {0x3ff0000000000001ULL, 0xbff0000000000001ULL,
                          0x7ff0000000000000ULL, 0xfff0000000000000ULL,
                          0x7ff8000000000000ULL, 0x7ff0000000000001ULL,
                          0xfff8000000001234ULL}) Write(status, 0x100, 8, bits, false);
    status = WithRule(WriteValueType::Float64, "1.5", "1.5");
    Write(status, 0x100, 8, 0x3ff8000000000000ULL, true);
    Write(status, 0x100, 8, 0x3ff8000000000001ULL, false);
}

void TestAddressesAndProtection()
{
    Rule(WriteValueType::U8, "0", "1", false, 0);
    Rule(WriteValueType::U8, "0", "1", true, UINT32_MAX);
    Rule(WriteValueType::U16, "0", "1", true, UINT32_MAX - 1);
    Rule(WriteValueType::U16, "0", "1", false, UINT32_MAX);
    Rule(WriteValueType::U64, "0", "1", true, UINT32_MAX - 7);
    Rule(WriteValueType::U64, "0", "1", false, UINT32_MAX - 6);

    WriteProtectionStatus status;
    Write(status, 0x100, 1, 0, false);
    status.readOnly = false;
    Write(status, 0, 1, 0, true);
    Write(status, 0x100, 8, UINT64_MAX, true);
    Write(status, UINT32_MAX, 1, 0, true);
    Write(status, UINT32_MAX - 7, 8, 0, true);
    Write(status, UINT32_MAX, 2, 0, false);
    Write(status, UINT32_MAX - 6, 8, 0, false);
    Write(status, UINT32_MAX, UINT16_MAX, 0, false);
    Write(status, 0x100, 0, 0, false);
    std::string error = "stale";
    CHECK(!ValidateProtectedWrite(status, 0x100, nullptr, 1, error));
    CHECK(!error.empty());

    status.restrictAddresses = true;
    status.firstAddress = 0x100;
    status.lastAddress = 0x107;
    Write(status, 0x100, 8, 0, true);
    Write(status, 0x107, 1, 0, true);
    Write(status, 0x106, 2, 0, true);
    Write(status, 0x107, 2, 0, false);
    Write(status, 0xff, 2, 0, false);
    Write(status, 0x108, 1, 0, false);
    status.firstAddress = 0x107;
    Write(status, 0x107, 1, 0, true);
    Write(status, 0x107, 2, 0, false);
    status.firstAddress = 0x108;
    Write(status, 0x107, 1, 0, false);
    status.restrictAddresses = false;
    Write(status, 0x107, 1, 0, true); // disabled range is ignored
    status.restrictAddresses = true;
    status.firstAddress = UINT32_MAX - 7;
    status.lastAddress = UINT32_MAX;
    Write(status, UINT32_MAX - 7, 8, 0, true);
    Write(status, UINT32_MAX, 1, 0, true);
    Write(status, UINT32_MAX, 2, 0, false);
    status.firstAddress = 0;
    status.lastAddress = 0;
    Write(status, 0, 1, 0, true);
    Write(status, 0, 2, 0, false);
}

void TestOverlapAndFailClosed()
{
    auto status = WithRule(WriteValueType::U32, "1", "2");
    Write(status, 0x100, 4, 1, true);
    Write(status, 0x100, 4, 3, false);
    for (uint32_t address = 0xfc; address < 0x108; ++address) {
        for (uint16_t size = 1; size <= 8; ++size) {
            const bool overlap = address < 0x104 && address + size > 0x100;
            const bool exact = address == 0x100 && size == 4;
            Write(status, address, size, 1, !overlap || exact);
        }
    }
    status.rules.push_back({0x104, WriteValueType::U16, "0", "10"});
    Write(status, 0x104, 2, 10, true);
    Write(status, 0x104, 2, 11, false);
    Write(status, 0x100, 6, 1, false); // covers two rules
    Write(status, 0x106, 1, 255, true); // adjacent, not overlapping

    // Defense in depth: evaluate every overlap even if a caller accidentally
    // supplies overlapping rules (the Add API normally prohibits them).
    status = WithRule(WriteValueType::U32, "0", "100");
    status.rules.push_back({0x100, WriteValueType::U32, "2", "3"});
    Write(status, 0x100, 4, 1, false);
    Write(status, 0x100, 4, 2, true);
    status.rules.push_back({0x101, WriteValueType::U8, "0", "255"});
    Write(status, 0x100, 4, 2, false);
    status.rules = {{0x200, static_cast<WriteValueType>(-1), "0", "1"}};
    Write(status, 0x100, 1, 0, false); // malformed unrelated config fails closed
    status.rules = {{0x200, WriteValueType::U8, "2", "1"}};
    Write(status, 0x100, 1, 0, false);
    status.rules = {{0, WriteValueType::U8, "0", "1"}};
    Write(status, 0x100, 1, 0, false);
    status = WithRule(WriteValueType::U64, "0", "1");
    status.rules[0].address = UINT32_MAX - 7;
    Write(status, UINT32_MAX - 7, 8, 1, true);
    Write(status, UINT32_MAX, 1, 0, false);
    status.readOnly = true;
    Write(status, UINT32_MAX - 7, 8, 1, false);
}
} // namespace

int main()
{
    TestTypesAndIntegerLimits();
    TestIntegerValues();
    TestFloats();
    TestAddressesAndProtection();
    TestOverlapAndFailClosed();
    std::cout << "Passed " << checks << " write-protection checks.\n";
}
