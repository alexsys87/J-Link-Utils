#!/usr/bin/env python3
"""Validate bilingual formats/IDs and exercise production language menu with real ImGui."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
LITERAL = r'"(?:\\.|[^"\\])*"'
STRINGS = rf'(?:{LITERAL}\s*)+'
PAIR = re.compile(rf'app_language::(?:Tr|Label)\(\s*({STRINGS}),\s*({STRINGS})\)')
FORMAT = re.compile(r'%(?:[-+ #0]*)(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspn%]')

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

sources = [ROOT / 'main.cpp', ROOT / 'include/imgui_memory_editor.h', ROOT / 'include/implot/implot.cpp']
sources += list((ROOT / 'Editor').glob('*.cpp')) + list((ROOT / 'Widget').glob('*Widget.*'))
pairs = []
for source in sources:
    text = source.read_text(encoding='utf-8-sig')
    # UI translations must never become XML tags/attributes or persisted keys.
    for call in ('FirstChildElement', 'NextSiblingElement', 'NewElement',
                 'SetAttribute', 'QueryIntAttribute', 'QueryUnsignedAttribute'):
        assert not re.search(r'\b' + call + r'\(\s*app_language::', text), (source, call)
    for en, ru in PAIR.findall(text):
        assert FORMAT.findall(en) == FORMAT.findall(ru), (source, en, ru)
        assert '\\0' not in en and '\\0' not in ru, 'NUL lists need explicit localized arrays'
        pairs.append('{' + en.strip() + ', ' + ru.strip() + '},')
assert len(pairs) >= 500, len(pairs)
ns = {'p': 'http://schemas.microsoft.com/developer/msbuild/2003'}
project = ET.parse(ROOT / 'J-Link Utils.vcxproj')
compilers = [group.find('p:ClCompile', ns) for group in project.findall('.//p:ItemDefinitionGroup', ns)
             if group.get('Condition')]
assert len(compilers) == 4
assert all('/utf-8' in item.find('p:AdditionalOptions', ns).text for item in compilers)
main = (ROOT / 'main.cpp').read_text(encoding='utf-8-sig')
assert 'ShowLanguageMenu();' in function(main, 'static void MenuWindow(')
with tempfile.TemporaryDirectory(prefix='localization-') as directory:
    build = Path(directory)
    (build / 'pairs.h').write_text('\n'.join(pairs))
    (build / 'language_menu.h').write_text(function(main, 'static void ShowLanguageMenu()'))
    command = [os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-g1', '-O0',
               '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
               '-I', str(build), '-I', str(ROOT), '-I', str(ROOT / 'include'),
               '-I', str(ROOT / 'include/imgui'), '-I', str(ROOT / 'Editor'),
               str(HERE / 'localization.cpp'), str(ROOT / 'Editor/WidgetTemplateFactory.cpp'),
               str(ROOT / 'include/tinyxml2/tinyxml2.cpp')]
    command += [str(ROOT / 'include/imgui' / name) for name in
                ('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')]
    binary = build / 'localization'
    subprocess.run([*command, '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(build / 'settings.ini')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
