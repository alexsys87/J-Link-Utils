// MemoryEditorWidget.cpp
#include "MemoryEditorWidget.h"

#if defined(_MSC_VER) && !defined(snprintf)
#define ImSnprintf  _snprintf
#else
#define ImSnprintf  snprintf
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define _PRISizeT   "I"
#else
#define _PRISizeT   "z"
#endif

MemoryEditorWidget::MemoryEditorWidget(const char* label,
    uint32_t base_addr,
    size_t size,
    int cols,
    bool read_only,
    bool show_ascii,
    bool show_hexii,
    bool show_options,
    bool show_preview,
    bool grey_zero,
    bool upper_hex,
    bool little_endian,
    bool use_external,
    bool use_cache)
    : m_label(label ? label : "")
    , m_base_addr(base_addr)
    , m_size(size)
    , m_cols(cols)
    , m_read_only(read_only)
    , m_show_ascii(show_ascii)
    , m_show_hexii(show_hexii)
    , m_show_options(show_options)
    , m_show_preview(show_preview)
    , m_grey_zero(grey_zero)
    , m_upper_hex(upper_hex)
    , m_little_endian(little_endian)
    , m_use_external(use_external)
    , m_use_cache(use_cache)
    , m_last_cache_update(0.0)
{
    if (!m_use_external && m_size > 0) {
        m_local_buffer.resize(m_size, 0);
    }
    if (m_use_external && m_use_cache && m_size > 0) {
        m_cache.resize(m_size, 0);
    }

    m_editor.ReadOnly = m_read_only;
    m_editor.Cols = m_cols;
    m_editor.OptShowAscii = m_show_ascii;
    m_editor.OptShowHexII = m_show_hexii;
    m_editor.OptShowOptions = m_show_options;
    m_editor.OptShowDataPreview = m_show_preview;
    m_editor.OptGreyOutZeroes = m_grey_zero;
    m_editor.OptUpperCaseHex = m_upper_hex;
    m_editor.OptMidColsCount = 8;
    m_editor.PreviewDataType = ImGuiDataType_U32;
    m_editor.PreviewEndianness = m_little_endian ? 0 : 1;
}

void MemoryEditorWidget::Draw(MainContext* ctx, uint32_t* dynamicAddr) {
    uint32_t currentBase = (dynamicAddr && *dynamicAddr) ? *dynamicAddr : m_base_addr;

    if (m_use_external && m_size > 0) {
        if (m_use_cache) {
            UpdateCache(ctx); // ˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜˜ DataReadWriteArray
        }

        CallbackData cbData = { this, ctx };
        m_editor.ReadFn = ReadCallback;
        m_editor.WriteFn = m_read_only ? nullptr : WriteCallback;
        m_editor.UserData = &cbData;

        m_editor.DrawContents(nullptr, m_size, currentBase);

        m_editor.UserData = nullptr;
    }
    else {
        // ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜
        m_editor.ReadFn = nullptr;
        m_editor.WriteFn = nullptr;
        m_editor.UserData = nullptr;
        if (m_local_buffer.size() < m_size) {
            m_local_buffer.resize(m_size, 0);
        }
        m_editor.DrawContents(m_local_buffer.data(), m_size, currentBase);
    }
}

// ˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ DataReadWriteArray
void MemoryEditorWidget::UpdateCache(MainContext* ctx) {
    double now = ImGui::GetTime();
    if (now - m_last_cache_update < CACHE_UPDATE_INTERVAL)
        return;

    m_last_cache_update = now;

    // ˜˜˜˜˜˜˜˜ ˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜
    uint16_t req_size = (m_size > 0xFFFF) ? 0xFFFF : (uint16_t)m_size;
    if (DataReadWriteArray(ctx, nullptr, nullptr, &m_base_addr, req_size) > 0) {
        if (m_base_addr < ctx->mcu_memory_base)
            return;
        uint32_t offset = m_base_addr - ctx->mcu_memory_base;
        if (offset >= IOMAP_SIZE || m_cache.empty())
            return;
        size_t copy_size = m_size;
        if ((size_t)offset + copy_size > IOMAP_SIZE)
            copy_size = IOMAP_SIZE - offset;
        if (copy_size > m_cache.size())
            copy_size = m_cache.size();
        SDL_LockMutex(ctx->Datalock);
        memcpy(m_cache.data(), ctx->IOMAP_Data + offset, copy_size);
        SDL_UnlockMutex(ctx->Datalock);
    }
}

// ˜˜˜˜˜˜ ˜˜˜˜˜ ˜˜ ˜˜˜˜ (˜˜˜˜ ˜˜˜ ˜˜˜˜˜˜˜) ˜˜˜ ˜˜˜˜˜˜˜˜
uint8_t MemoryEditorWidget::ReadByteCached(MainContext* ctx, uint32_t phys_addr) {
    if (!m_use_cache) {
        return DataReadWriteU08(ctx, nullptr, nullptr, &phys_addr);
    }
    uint32_t off = phys_addr - m_base_addr;
    if (off < m_size)
        return m_cache[off];
    return 0;
}

// ˜˜˜˜˜˜ ˜˜˜˜˜ ˜ ˜˜˜˜˜˜˜˜˜˜˜˜ ˜˜˜˜
void MemoryEditorWidget::WriteByteCached(MainContext* ctx, uint32_t phys_addr, uint8_t val) {
    DataReadWriteU08(ctx, nullptr, &val, &phys_addr);
    if (m_use_cache) {
        uint32_t off = phys_addr - m_base_addr;
        if (off < m_size)
            m_cache[off] = val; // ˜˜˜˜˜ ˜˜˜˜˜˜˜˜˜ ˜˜˜
        else
            m_last_cache_update = 0.0; // ˜˜˜ ˜˜˜˜˜˜ ˜˜ ˜˜˜˜˜˜˜ ˜˜˜˜˜˜˜ ˜˜˜
    }
}

// ˜˜˜˜˜˜˜˜˜˜˜ callback ˜˜˜ ˜˜˜˜˜˜
ImU8 MemoryEditorWidget::ReadCallback(const ImU8* /*mem*/, size_t off, void* user_data) {
    CallbackData* data = static_cast<CallbackData*>(user_data);
    if (!data || !data->widget || !data->ctx) return 0;
    MemoryEditorWidget* self = data->widget;
    if (off >= self->m_size) return 0;

    uint32_t phys_addr = self->m_base_addr + (uint32_t)off;
    return self->ReadByteCached(data->ctx, phys_addr);
}

// ˜˜˜˜˜˜˜˜˜˜˜ callback ˜˜˜ ˜˜˜˜˜˜
void MemoryEditorWidget::WriteCallback(ImU8* /*mem*/, size_t off, ImU8 d, void* user_data) {
    CallbackData* data = static_cast<CallbackData*>(user_data);
    if (!data || !data->widget || !data->ctx) return;
    MemoryEditorWidget* self = data->widget;
    if (off >= self->m_size) return;

    uint32_t phys_addr = self->m_base_addr + (uint32_t)off;
    self->WriteByteCached(data->ctx, phys_addr, (uint8_t)d);
}