// gif_loader.cpp
#define GIF_LOAD_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244) // отключаем предупреждение о потере данных
#include "gif_load.h"
#pragma warning(pop)

#include <SDL2/SDL.h>
#include <vector>
#include <cstdio>

struct GifLoadContext {
    SDL_Renderer* renderer;
    std::vector<SDL_Texture*>& textures;
    std::vector<int>& delays;
};

static void GifFrameWriter(void* anim, struct GIF_WHDR* whdr) {
    GifLoadContext* ctx = (GifLoadContext*)anim;
    if (!ctx || !whdr || !whdr->bptr) return;

    int width = whdr->frxd;
    int height = whdr->fryd;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return;

    uint8_t* pixels = (uint8_t*)surf->pixels;
    int pitch = surf->pitch;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            uint8_t palIdx = whdr->bptr[idx];
            uint32_t color = 0x00000000;
            if (palIdx < whdr->clrs) {
                uint8_t r = whdr->cpal[palIdx].R;
                uint8_t g = whdr->cpal[palIdx].G;
                uint8_t b = whdr->cpal[palIdx].B;
                uint8_t a = (palIdx == whdr->tran) ? 0 : 255;
                color = (a << 24) | (r << 16) | (g << 8) | b;
            }
            uint32_t* pixel = (uint32_t*)(pixels + y * pitch + x * 4);
            *pixel = color;
        }
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    SDL_FreeSurface(surf);

    if (tex) {
        ctx->textures.push_back(tex);
        int delay = whdr->time * 10;
        if (delay <= 0) delay = 100;
        ctx->delays.push_back(delay);
    }
}

extern "C" {
    int LoadGifFrames(SDL_Renderer* renderer, const char* filename,
        std::vector<SDL_Texture*>& outTextures,
        std::vector<int>& outDelays) {
        if (!renderer || !filename) return 0;

        char* base_path = SDL_GetBasePath();
        if (!base_path) return 0;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", base_path, filename);
        SDL_free(base_path);

        FILE* f = nullptr;
        errno_t err = fopen_s(&f, full_path, "rb");
        if (err != 0 || !f) return 0;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> buffer(fsize);
        fread(buffer.data(), 1, fsize, f);
        fclose(f);

        GifLoadContext ctx = { renderer, outTextures, outDelays };
        long result = GIF_Load(buffer.data(), fsize, GifFrameWriter, nullptr, &ctx, 0);
        return (int)result;
    }
}