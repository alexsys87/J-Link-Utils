#!/usr/bin/env python3
"""Compile the production font loader with real ImGui/FreeType, without J-Link.

Requires g++, pkg-config, libfreetype-dev and fonts-dejavu-core.
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
# Check wiring, as the headless test cannot create a Windows HWND.
assert source.index('SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2")') < source.index('if (SDL_Init(')
assert 'getDpiForWindow(wmInfo.info.win.window)' in source
assert source.index('FontScaleDpi = GetDPIScale(ctx->Wimdow)') < source.index('ImGui::NewFrame();')
assert 'io.Fonts->Build()' not in loader

with tempfile.TemporaryDirectory(prefix='font-rendering-') as directory:
    build = Path(directory)
    (build / 'loader.h').write_text(loader)
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'freetype2'], text=True))
    command = [os.environ.get('CXX', 'g++'), '-std=c++17', '-g', '-O0',
               '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
               '-fno-pie', '-no-pie', '-I', str(build),
               '-I', str(ROOT / 'include/imgui'),
               str(HERE / 'font_rendering.cpp')]
    command += [str(ROOT / 'include/imgui' / name) for name in (
        'imgui.cpp', 'imgui_draw.cpp', 'imgui_widgets.cpp', 'imgui_tables.cpp',
        'misc/freetype/imgui_freetype.cpp')]
    command += flags + ['-o', str(build / 'font_rendering')]
    subprocess.run(command, check=True)
    subprocess.run([str(build / 'font_rendering')], check=True, env={
        **os.environ, 'ASAN_OPTIONS': 'detect_leaks=1',
        'UBSAN_OPTIONS': 'halt_on_error=1'})
