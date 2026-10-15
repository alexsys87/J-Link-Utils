// No J-Link headers, import library, hardware, display or driver required.
#include <SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <png.h>
#include <bzlib.h>
#include <zlib.h>
#include <brotli/decode.h>
#include <brotli/encode.h>

int main(int, char**)
{
    SDL_version version{};
    SDL_GetVersion(&version);
    if (version.major != SDL_MAJOR_VERSION || SDL_Init(SDL_INIT_TIMER) != 0) return 1;
    SDL_Quit();
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) return 2;
    FT_Done_FreeType(library);
    if (!png_access_version_number() || !BZ2_bzlibVersion() || !zlibVersion()) return 3;
    return BrotliDecoderVersion() && BrotliEncoderVersion() ? 0 : 4;
}
