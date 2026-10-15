#!/usr/bin/env python3
"""Check production fonts and full DPI rendering with real ImGui/FreeType/SDL.

Requires g++, pkg-config, libfreetype-dev, libsdl2-dev and fonts-dejavu-core.
Run: python3 tests/font_rendering/run.py
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


source = (ROOT / 'main.cpp').read_text(encoding='utf-8-sig')
loader = function(source, 'void LoadFonts(MainContext* ctx)')
# Check Windows-specific wiring; Linux cannot exercise WM_DPICHANGED itself.
configuration = function(source, 'void ConfigureDpiAwareness()')
frame = function(source, 'void PrepareUiFrame(SDL_Renderer* renderer)')
hit_test = function(source, 'static SDL_HitTestResult SDLCALL MainWindowHitTest(')
assert 'SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2")' in configuration
assert 'SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1")' in configuration
entry = function(source, 'int main(int argc, char* argv[])')
assert entry.index('ConfigureDpiAwareness();') < entry.index('SDL_ShowSimpleMessageBox(')
assert entry.index('ConfigureDpiAwareness();') < entry.index('if (SDL_Init(')
assert 'FontScaleDpi = 1.0f' in frame
assert 'GetDPIScale(' not in source
assert 'io.MousePos =' not in entry and 'io.MouseDown[' not in entry
assert entry.count('ImGui_ImplSDL2_ProcessEvent(&event)') == 1
assert 'PrepareUiFrame(ctx->Renderer);' in entry
assert 'io.Fonts->Build()' not in loader

with tempfile.TemporaryDirectory(prefix='font-rendering-') as directory:
    build = Path(directory)
    (build / 'loader.h').write_text(configuration + frame + hit_test + loader)
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'freetype2', 'sdl2'], text=True))
    command = [os.environ.get('CXX', 'g++'), '-std=c++17', '-g', '-O0',
               '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
               '-fno-pie', '-no-pie', '-I', str(build),
               '-I', str(ROOT / 'include/imgui'),
               str(HERE / 'font_rendering.cpp')]
    command += [str(ROOT / 'include/imgui' / name) for name in (
        'imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp',
        'misc/freetype/imgui_freetype.cpp', 'backends/imgui_impl_sdl2.cpp',
        'backends/imgui_impl_sdlrenderer2.cpp')]
    command += flags + ['-o', str(build / 'font_rendering')]
    subprocess.run(command, check=True)
    subprocess.run([str(build / 'font_rendering')], check=True, env={
        **os.environ, 'ASAN_OPTIONS': 'detect_leaks=1',
        'UBSAN_OPTIONS': 'halt_on_error=1', 'SDL_VIDEODRIVER': 'dummy',
        'SDL_AUDIODRIVER': 'dummy'})
