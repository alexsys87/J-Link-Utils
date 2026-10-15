// CaptureHandshake.h
// Buffered capture used by DataLog and Histogram: the MCU fills its arrays and
// sets a non-zero ready flag (DoneAddr). The arrays are taken only after a read
// that started after the flag was seen has completed, and only then is the flag
// cleared, so the MCU cannot overwrite an array while it is being read.
// Include after widget_common.h.
#pragma once

class CaptureHandshake {
public:
    // Returns true when the arrays in the cache hold a complete capture.
    // subscribe() must request every array of the widget.
    template <class Subscribe>
    bool Update(MainContext* ctx, uint32_t flagAddr, Subscribe subscribe) {
        if (!ctx->StartComunication) {
            m_reading = false;
            return false;
        }
        if (flagAddr == 0) {
            // No handshake: the arrays are shown live.
            subscribe();
            return true;
        }
        uint32_t addr = flagAddr;
        const uint32_t flag = DataReadWriteU32(ctx, nullptr, nullptr, &addr);
        if (!m_reading) {
            if (flag == 0) return false;
            subscribe();
            m_cycle = DataCompletedCycles();
            m_reading = true;
            return false;
        }
        subscribe();
        // The cycle running when the arrays were requested may not include them.
        if (DataCompletedCycles() - m_cycle < 2) return false;
        uint32_t zero = 0;
        DataReadWriteU32(ctx, nullptr, &zero, &addr);
        m_reading = false;
        return true;
    }

private:
    bool m_reading = false;
    uint32_t m_cycle = 0;
};
