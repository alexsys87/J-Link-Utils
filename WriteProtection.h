#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct MainContext;
enum class WriteValueType { U8, U16, U32, U64, I8, I16, I32, I64, Float32, Float64 };
struct WriteValueRule {
    uint32_t address = 0;
    WriteValueType type = WriteValueType::U16;
    std::string minimum;
    std::string maximum;
};
struct WriteProtectionStatus {
    bool readOnly = true;
    bool restrictAddresses = false;
    uint32_t firstAddress = 0;
    uint32_t lastAddress = UINT32_MAX; // inclusive
    std::vector<WriteValueRule> rules;
    uint64_t blockedWrites = 0;
    uint64_t epoch = 0; // snapshot token for a session-bound unlock dialog
    std::string lastError;
};

unsigned WriteValueSize(WriteValueType type);
bool ValidateWriteValueRule(const WriteValueRule& rule, std::string& error);
bool ValidateProtectedWrite(const WriteProtectionStatus& status, uint32_t address,
    const uint8_t* bytes, uint16_t size, std::string& error);

WriteProtectionStatus GetWriteProtectionStatus();
void LockTargetWrites(MainContext* ctx);
// UI calls only after the explicit session-unlock confirmation.
bool ArmTargetWrites(MainContext* ctx, uint64_t expectedEpoch = UINT64_MAX);
bool SetWriteAddressRange(MainContext* ctx, bool enabled, uint32_t first, uint32_t last);
bool AddWriteValueRule(MainContext* ctx, const WriteValueRule& rule);
void RemoveWriteValueRule(MainContext* ctx, unsigned index);
