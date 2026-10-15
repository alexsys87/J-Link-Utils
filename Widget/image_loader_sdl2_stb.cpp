// image_loader_sdl2_stb.cpp
#include <SDL2/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"

extern "C" {

    ImTextureID LoadTextureFromFile(SDL_Renderer* renderer, const char* filename, int* out_width, int* out_height) {
        if (!renderer || !filename) return (ImTextureID)0;

        // 1. Folder of the EXE
        char* base_path = SDL_GetBasePath();
        if (!base_path) return (ImTextureID)0;

        // 2. Full path (base_path already ends with a slash)
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", base_path, filename);
        SDL_free(base_path); // free right after joining

        // 3. Load with stb_image
        int w, h, channels;
        unsigned char* pixels = stbi_load(full_path, &w, &h, &channels, 4);
        if (!pixels) return (ImTextureID)0;

        // 4. Create the SDL texture
        SDL_Texture* texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STATIC,
            w, h);

        if (!texture) {
            stbi_image_free(pixels);
            return (ImTextureID)0;
        }

        SDL_UpdateTexture(texture, nullptr, pixels, w * 4);
        stbi_image_free(pixels);

        if (out_width) *out_width = w;
        if (out_height) *out_height = h;

        return (ImTextureID)texture;
    }

    void FreeTexture(ImTextureID tex) {
        if (tex != (ImTextureID)0) {
            SDL_DestroyTexture((SDL_Texture*)tex);
        }
    }

} // extern "C"