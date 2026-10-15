#!/usr/bin/env python3
"""Round-trip production style XML through UTF-8 paths, without SDL/J-Link."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'main.cpp').read_text(encoding='utf-8-sig')

def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

opener = function('static FILE* OpenStyleFile(')
assert '_wfopen_s(' in opener and 'std::filesystem::u8path(filename).native()' in opener
save = function('bool SaveStyleToXML(')
load = function('bool LoadStyleFromXML(')
assert 'doc.SaveFile(file)' in save and 'doc.LoadFile(file)' in load
assert 'fclose(file)' in save and 'fclose(file)' in load

with tempfile.TemporaryDirectory(prefix='style-persistence-') as directory:
    build = Path(directory)
    test = build / 'test.cpp'
    test.write_text(r'''
#include <imgui.h>
#include <tinyxml2/tinyxml2.h>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <cassert>
''' + opener + save + load + r'''
int main(int argc, char** argv) {
    assert(argc == 2);
    ImGui::CreateContext();
    ImGuiStyle initial = ImGui::GetStyle();
    initial.WindowPadding = ImVec2(17.25f, 11.5f);
    initial.FrameRounding = 7.0f;
    initial.Colors[ImGuiCol_Text] = ImVec4(.1f, .2f, .3f, .4f);
    const auto folder = std::filesystem::u8path(argv[1]) / std::filesystem::u8path(u8"Настройки 日本語");
    std::filesystem::create_directories(folder);
    const auto path = (folder / "custom.style.xml").u8string();
    assert(SaveStyleToXML(path.c_str(), initial));
    ImGuiStyle restored = ImGui::GetStyle();
    assert(LoadStyleFromXML(path.c_str(), restored));
    assert(restored.WindowPadding.x == initial.WindowPadding.x);
    assert(restored.WindowPadding.y == initial.WindowPadding.y);
    assert(restored.FrameRounding == initial.FrameRounding);
    assert(restored.Colors[ImGuiCol_Text].w == initial.Colors[ImGuiCol_Text].w);
    const auto missing = (folder / "missing" / "style.xml").u8string();
    assert(!SaveStyleToXML(missing.c_str(), initial));
    assert(!LoadStyleFromXML(missing.c_str(), restored));
    ImGui::DestroyContext();
    puts("Style XML: Unicode path round-trip and I/O failures passed");
}
''', encoding='utf-8')
    compiler = os.environ.get('CXX', 'g++')
    binary = build / 'style_test'
    command = [compiler, '-std=c++17', '-O0', '-g1', '-fsanitize=address,undefined',
               '-fno-pie', '-no-pie', '-I', str(ROOT / 'include'),
               '-I', str(ROOT / 'include/imgui'), str(test),
               str(ROOT / 'include/tinyxml2/tinyxml2.cpp')]
    command += [str(ROOT / 'include/imgui' / name) for name in (
        'imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')]
    subprocess.run(command + ['-o', str(binary)], check=True)
    subprocess.run([str(binary), directory], check=True, env={
        **os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
