#!/usr/bin/env python3
"""Check guide coverage/filtering and render the real guide with Dear ImGui.

No SDL, Windows, graphics device or J-Link is needed. Requires Python 3 and g++.
Run from any directory: python3 tests/user_guide/run.py
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

# New loader tags must also be mentioned in the catalogue. Read both sources,
# rather than maintaining a separate hard-coded list of widgets in this test.
loader = (ROOT / 'xml_window.cpp').read_text(encoding='utf-8-sig')
guide = (ROOT / 'UserGuide.h').read_text()
tags = set(re.findall(r'name == "(\w+)"', loader))
catalogue = guide.split('{"8. Widget catalogue", R"guide(', 1)[1].split(')guide"}', 1)[0]
missing = tags - set(re.findall(r'\b\w+\b', catalogue))
assert not missing, f'Widgets missing from User Guide: {sorted(missing)}'
assert f'all {len(tags)} supported XML widget tags' in catalogue

# The copyable minimal example must be well-formed and use the loader's actual
# top-level Window/Items shape (not an unsupported wrapper).
example = guide.split('<Window>', 1)[1].split('</Window>', 1)[0]
window = ET.fromstring('<Window>' + example + '</Window>')
assert window.find('Text') is not None and window.find('Items') is not None
assert all(widget.tag in tags for widget in window.find('Items'))

main = (ROOT / 'main.cpp').read_text(encoding='utf-8-sig')
assert '#include "UserGuide.h"' in main and 'user_guide::Show(p_open);' in main
for name in ('J-Link Utils.vcxproj', 'J-Link Utils.vcxproj.filters'):
    tree = ET.parse(ROOT / name)
    ns = {'p': 'http://schemas.microsoft.com/developer/msbuild/2003'}
    assert tree.find('.//p:ClInclude[@Include="UserGuide.h"]', ns) is not None

with tempfile.TemporaryDirectory(prefix='user-guide-') as directory:
    binary = Path(directory) / 'user_guide'
    compiler = os.environ.get('CXX', 'g++')
    command = [compiler, '-std=c++17', '-g1', '-O0', '-fno-omit-frame-pointer',
               '-fsanitize=address,undefined', '-fno-sanitize-recover=undefined',
               '-fno-pie', '-no-pie', '-Wall', '-Wextra',
               '-I', str(ROOT), '-I', str(ROOT / 'include/imgui'),
               str(HERE / 'user_guide.cpp')]
    command += [str(ROOT / 'include/imgui' / name) for name in
                ('imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')]
    subprocess.run([*command, '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1',
                        'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})
print(f'Guide catalogue covers all {len(tags)} loader tags; XML/project checks passed')
