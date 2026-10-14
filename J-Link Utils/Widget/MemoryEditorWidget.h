// MemoryEditorWidget.h
#pragma once
#include "widget_common.h"
#include "imgui_memory_editor.h"
#include <vector>

class MemoryEditorWidget : public UIWidget {
public:
    MemoryEditorWidget(const char* label,
        uint32_t base_addr,
        size_t size,
        int cols = 16,
        bool read_only = false,
        bool show_ascii = true,
        bool show_hexii = false,
        bool show_options = true,
        bool show_preview = true,
        bool grey_zero = true,
        bool upper_hex = true,
        bool little_endian = true,
        bool use_external = false,
        bool use_cache = true);      // новый параметр: использовать кэширование

    virtual ~MemoryEditorWidget() = default;
    virtual void Draw(MainContext* ctx, uint32_t* dynamicAddr = nullptr) override;

private:
    std::string         m_label;
    uint32_t            m_base_addr;        // базовый физический адрес
    size_t              m_size;             // размер буфера в байтах
    int                 m_cols;
    bool                m_read_only;
    bool                m_show_ascii;
    bool                m_show_hexii;
    bool                m_show_options;
    bool                m_show_preview;
    bool                m_grey_zero;
    bool                m_upper_hex;
    bool                m_little_endian;
    bool                m_use_external;     // true Ц внешн€€ пам€ть, false Ц локальный буфер
    bool                m_use_cache;        // true Ц использовать кэширование

    std::vector<uint8_t> m_local_buffer;    // локальные данные (если не внешние)
    std::vector<uint8_t> m_cache;           // кэш внешних данных
    double              m_last_cache_update;
    static constexpr double CACHE_UPDATE_INTERVAL = 0.1; // 100 мс

    MemoryEditor        m_editor;

    // —труктура дл€ передачи данных в callback'и
    struct CallbackData {
        MemoryEditorWidget* widget;
        MainContext* ctx;
    };

    // —татические callback-функции
    static ImU8 ReadCallback(const ImU8* mem, size_t off, void* user_data);
    static void WriteCallback(ImU8* mem, size_t off, ImU8 d, void* user_data);

    // ¬нутренние методы дл€ работы с кэшем
    void UpdateCache(MainContext* ctx);
    uint8_t ReadByteCached(MainContext* ctx, uint32_t phys_addr);
    void WriteByteCached(MainContext* ctx, uint32_t phys_addr, uint8_t val);
};