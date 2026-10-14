#!/usr/bin/env python3
"""Run headless production-widget regressions with ASan and UBSan.

Only widget_common.h/GUI includes are replaced by test doubles in a temporary
copy; widget implementations remain unchanged. Editor pure functions are copied
verbatim from the production source so its Windows GUI dependency is unnecessary.
Run: python3 tests/ui_regression/run.py
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SRC = ROOT / 'J-Link Utils'

def function(source, signature):
    start = source.index(signature)
    opening = source.index(b'{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == ord('{')) - (source[end] == ord('}'))
        end += 1
    return source[start:end] + b'\n'

with tempfile.TemporaryDirectory(prefix='ui-regression-') as directory:
    build = Path(directory)
    shutil.copyfile(HERE / 'ui_stubs.h', build / 'ui_stubs.h')
    shutil.copyfile(SRC / 'Widget/ScalarFormat.h', build / 'ScalarFormat.h')
    shutil.copyfile(SRC / 'Widget/MemoryBounds.h', build / 'MemoryBounds.h')
    shutil.copyfile(SRC / 'Editor/XMLSafety.h', build / 'XMLSafety.h')
    # Read the list of actual test includes, instead of maintaining a second list.
    test_source = (HERE / 'ui_regression.cpp').read_bytes()
    for name in re.findall(rb'#include "([^"]+Widget\.h)"', test_source):
        name = name.decode('ascii')
        header = (SRC / 'Widget' / name).read_bytes()
        header = header.replace(b'#include "widget_common.h"', b'#include "ui_stubs.h"')
        (build / name).write_bytes(header)
    for name in ('imgui_internal.h', 'implot.h'):
        (build / name).write_text('#pragma once\n#include "ui_stubs.h"\n')
    editor = (SRC / 'Editor/XMLEditor.cpp').read_bytes()
    parsing = b'\n'.join(function(editor, signature) for signature in (
        b'static bool ParseColor(', b'static bool IsBooleanString(', b'static bool IsNumberString('))
    (build / 'editor_parsing_under_test.h').write_bytes(parsing)
    # Protect the lifetime fix at the actual UI call sites, too.
    assert b'g_selectedNode->GetDocument()' not in editor
    assert b'!ui_safety::ContainsXMLNode(ctx->CurrentXMLDoc, g_selectedNode)' in editor
    assert b'DeepClone(doc)' not in editor
    deletion = editor.index(b'if (ImGui::Button("Delete this node"))')
    assert b'return;' in editor[deletion:editor.index(b'ImGui::BeginPopup("AddChildWidgetPopup")', deletion)]
    xml = (SRC / 'xml_window.cpp').read_bytes()
    assert not re.search(rb'FirstChildElement\("\w+"\) \? \w+->FirstChildElement\("\w+"\)->GetText\(\) : "', xml)
    assert b'lbl, (uint16_t)addr' not in xml
    compiler = os.environ.get('CXX', 'g++')
    command = [compiler, '-std=c++17', '-g', '-O1', '-fno-omit-frame-pointer',
               '-fsanitize=address,undefined', '-fno-pie', '-no-pie',
               '-I', str(build), '-I', str(SRC / 'include'),
               str(HERE / 'ui_regression.cpp'),
               str(SRC / 'include/tinyxml2/tinyxml2.cpp'),
               '-o', str(build / 'ui_regression')]
    subprocess.run(command, check=True)
    subprocess.run([str(build / 'ui_regression')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    # Also compile the formatter with Main.h's macro active before inclusion.
    macro_test = build / 'format_macro.cpp'
    macro_test.write_text('#define snprintf _snprintf_s\n#include "ScalarFormat.h"\n'
                          '#ifndef snprintf\n#error Macro not restored\n#endif\n'
                          'int main(){char b[16];ui_safety::FormatScalar(b,16,"%d",42);return b[0]!=\'4\';}\n')
    subprocess.run([compiler, '-std=c++17', '-I', str(build), str(macro_test), '-o', str(build / 'format_macro')], check=True)
    subprocess.run([str(build / 'format_macro')], check=True)
    print('Formatter Windows-macro compatibility test passed')
