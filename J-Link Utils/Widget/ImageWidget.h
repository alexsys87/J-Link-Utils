// ImageWidget.h
#pragma once
#include "widget_common.h"

#ifdef __cplusplus
extern "C" {
#endif
    extern ImTextureID LoadTextureFromFile(SDL_Renderer* renderer, const char* filename, int* out_width, int* out_height);
    extern void FreeTexture(ImTextureID tex);
#ifdef __cplusplus
}
#endif

class ImageWidget : public LeafWidget {
    std::string filePath;
    float scale;
    float fixedWidth, fixedHeight;  // если >0, то фиксированный размер
    ImVec2 uv0, uv1;
    ImU32 tint_col;
    ImU32 border_col;

    ImTextureID texID;
    int texWidth, texHeight;
    bool texLoaded;

public:
    ImageWidget(const char* lbl, const char* path, float s,
        float fixW = 0, float fixH = 0,
        ImVec2 u0 = ImVec2(0, 0), ImVec2 u1 = ImVec2(1, 1),
        ImU32 tint = IM_COL32(255, 255, 255, 255),
        ImU32 border = 0)
        : LeafWidget(lbl), filePath(path ? path : ""), scale(s),
        fixedWidth(fixW), fixedHeight(fixH),
        uv0(u0), uv1(u1), tint_col(tint), border_col(border),
        texID(ImTextureID()), texWidth(0), texHeight(0), texLoaded(false) {
    }

    ~ImageWidget() {
        FreeTextureIfLoaded();
    }

    void Draw(MainContext* ctx, uint32_t* dynamicAddr) override {
        (void)dynamicAddr;

        if (!texLoaded && !filePath.empty()) {
            texID = LoadTextureFromFile(ctx->Renderer, filePath.c_str(), &texWidth, &texHeight);
            texLoaded = (texID != ImTextureID());
        }

        if (!texLoaded) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Image not loaded: %s", filePath.c_str());
            return;
        }

        float w, h;
        if (fixedWidth > 0 && fixedHeight > 0) {
            w = fixedWidth;
            h = fixedHeight;
        }
        else {
            w = texWidth * scale;
            h = texHeight * scale;
        }

        if (!text.empty()) {
            ImGui::Text("%s", text.c_str());
        }

        ImGui::Image(texID, ImVec2(w, h), uv0, uv1, ImColor(tint_col), ImColor(border_col));

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", filePath.c_str());
        }
    }

private:
    void FreeTextureIfLoaded() {
        if (texLoaded && texID != ImTextureID()) {
            FreeTexture(texID);
            texID = ImTextureID();
            texLoaded = false;
        }
    }
};