// GifWidget.h
#pragma once
#include "widget_common.h"
#include <vector>
#include <cstdint>

// GIF loading (implemented in a separate .cpp)
#ifdef __cplusplus
extern "C" {
#endif
    // Loads all GIF frames; returns the textures and the frame delays (ms).
    // Returns the number of frames, or 0 on error.
    int LoadGifFrames(SDL_Renderer* renderer, const char* filename,
        std::vector<SDL_Texture*>& outTextures,
        std::vector<int>& outDelays);
#ifdef __cplusplus
}
#endif

class GifWidget : public LeafWidget {
    std::string filePath;
    float fixedWidth, fixedHeight; // fixed size if > 0, otherwise 1:1
    float scale;                   // scale when no fixed size is set
    bool loop;                     // loop the animation
    float speedFactor;             // speed factor (1.0 = original)

    std::vector<SDL_Texture*> frames;
    std::vector<int> delays;       // delays in milliseconds
    int currentFrame;
    double nextFrameTime;          // ImGui time of the next frame switch
    bool loaded;
    bool triedLoad = false;

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

        // One attempt only: a missing or broken file is not reread every frame,
        // and a partly decoded file does not add textures again and again.
        if (!triedLoad && !filePath.empty()) {
            triedLoad = true;
            loaded = (LoadGifFrames(ctx->Renderer, filePath.c_str(), frames, delays) > 0);
            if (!loaded) {
                for (auto tex : frames) {
                    if (tex) SDL_DestroyTexture(tex);
                }
                frames.clear();
                delays.clear();
            }
        }

        if (!loaded || frames.empty()) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "GIF not loaded: %s", filePath.c_str());
            return;
        }

        double now = ImGui::GetTime();
        if (now >= nextFrameTime) {
            // Next frame
            int next = currentFrame + 1;
            if (next >= (int)frames.size()) {
                if (loop) next = 0;
                else next = currentFrame; // stay on the last frame
            }
            if (next != currentFrame) {
                currentFrame = next;
                // Time of the next switch
                int delay = delays[currentFrame];
                if (delay < 20) delay = 100; // use 100 ms for very short delays (some GIFs have 0)
                nextFrameTime = now + (delay * speedFactor) / 1000.0;
            }
            else {
                // No switch: still update the time to avoid spinning
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

        // ImGui::Image takes an ImTextureID; with SDL2 it is the texture pointer
        ImGui::Image((ImTextureID)tex, ImVec2(dispW, dispH));

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", filePath.c_str());
        }
    }
};