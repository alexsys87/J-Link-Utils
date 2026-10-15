// GifWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <cstdint>

// Функции для загрузки GIF (объявляем, реализация в отдельном .cpp)
#ifdef __cplusplus
extern "C" {
#endif
    // Загружает все кадры GIF, возвращает массив текстур и массив задержек (в мс).
    // Возвращает количество кадров, или 0 при ошибке.
    int LoadGifFrames(SDL_Renderer* renderer, const char* filename,
        std::vector<SDL_Texture*>& outTextures,
        std::vector<int>& outDelays);
#ifdef __cplusplus
}
#endif

class GifWidget : public LeafWidget {
    std::string filePath;
    float fixedWidth, fixedHeight; // если >0, фиксированный размер, иначе масштаб 1:1
    float scale;                   // масштаб, если фиксированный размер не задан
    bool loop;                     // зацикливать ли анимацию
    float speedFactor;             // множитель скорости (1.0 = оригинальная)

    std::vector<SDL_Texture*> frames;
    std::vector<int> delays;       // задержки в миллисекундах
    int currentFrame;
    double nextFrameTime;          // время ImGui, когда нужно переключить кадр
    bool loaded;

public:
    GifWidget(const char* lbl, const char* path,
        float fixW = 0, float fixH = 0,
        float s = 1.0f, bool lp = true, float speed = 1.0f)
        : LeafWidget(lbl), filePath(path ? path : ""),
        fixedWidth(fixW), fixedHeight(fixH), scale(s),
        loop(lp), speedFactor(speed),
        currentFrame(0), nextFrameTime(0), loaded(false) {
    }

    ~GifWidget() {
        for (auto tex : frames) {
            if (tex) SDL_DestroyTexture(tex);
        }
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        (void)dynamicAddr;

        if (!loaded && !filePath.empty()) {
            loaded = (LoadGifFrames(ctx->Renderer, filePath.c_str(), frames, delays) > 0);
        }

        if (!loaded || frames.empty()) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "GIF not loaded: %s", filePath.c_str());
            return;
        }

        double now = ImGui::GetTime();
        if (now >= nextFrameTime) {
            // Переключаем на следующий кадр
            int next = currentFrame + 1;
            if (next >= (int)frames.size()) {
                if (loop) next = 0;
                else next = currentFrame; // остаёмся на последнем
            }
            if (next != currentFrame) {
                currentFrame = next;
                // Устанавливаем время следующего переключения
                int delay = delays[currentFrame];
                if (delay < 20) delay = 100; // минимальная задержка 100 мс (некоторые GIF могут иметь 0)
                nextFrameTime = now + (delay * speedFactor) / 1000.0;
            }
            else {
                // Если не переключились, всё равно обновим время, чтобы не спамить
                nextFrameTime = now + 0.1;
            }
        }

        SDL_Texture* tex = frames[currentFrame];
        int w, h;
        SDL_QueryTexture(tex, NULL, NULL, &w, &h);

        float dispW, dispH;
        if (fixedWidth > 0 && fixedHeight > 0) {
            dispW = fixedWidth;
            dispH = fixedHeight;
        }
        else {
            dispW = w * scale;
            dispH = h * scale;
        }

        if (!text.empty()) {
            ImGui::Text("%s", text.c_str());
        }

        // ImGui::Image требует ImTextureID, который в SDL2 можно получить как (void*)tex
        ImGui::Image((ImTextureID)tex, ImVec2(dispW, dispH));

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", filePath.c_str());
        }
    }
};