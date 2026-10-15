#!/usr/bin/env python3
"""Load and draw every widget the XML loader supports with real Dear ImGui and ImPlot.

Each widget is loaded with no properties, with the editor's template and with hostile
values (zero, negative, huge, nan, inf, malformed text) for every property, then drawn
for several frames with changing target memory and mouse input. The XML editor's
property panel is drawn for every element of those documents as well. Any ImGui
assertion, sanitizer report or crash fails the test.

Production sources are compiled unchanged; only Main.h is replaced by a copy without
the Windows-only includes, and SDL/J-Link/data-exchange functions are test doubles.
A static check also verifies that the editor's widget table lists every property
the loader reads.
Run: python3 tests/widget_smoke/run.py
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def function(source, signature):
    """Text of a top-level function, from its signature to the closing brace."""
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


# Variable that holds a list item in the loader, per (widget, item)
ITEM_VARIABLES = {('PlotLines', 'Line'): 'e', ('Histogram', 'Line'): 'lineElem', ('DataLog', 'Line'): 'lineElem',
                  ('RadioButton', 'Radio'): 'radio', ('ComboBox', 'Combo'): 'item',
                  ('SegmentButton', 'Segment'): 'segElem', ('RPMIndicator', 'Zone'): 'zoneElem'}


def reads(branch, variable):
    """Properties the branch reads from the element in 'variable' (child elements or attributes)."""
    v = re.escape(variable)
    props = set(re.findall(r'Read\w*\(' + v + r', "(\w+)"', branch))
    props |= set(re.findall(r'XmlChildText\(' + v + r', "(\w+)"', branch))
    props |= set(re.findall(v + r'->FirstChildElement\("(\w+)"\)', branch))
    props |= set(re.findall(v + r'->Query\w*Attribute\("(\w+)"', branch))
    props |= set(re.findall(v + r'->Attribute\("(\w+)"\)', branch))
    return props - {'Items', 'Zones'}


def parser_tags():
    """Widget tag (or "Widget/Item") -> property tags the loader reads."""
    source = (ROOT / 'xml_window.cpp').read_text()
    body = function(source, 'static std::unique_ptr<UIWidget> LoadWidgetFromXML(XMLElement* elem) {')
    tags = {}
    # Each branch starts with name == "X" (possibly several names)
    branches = re.split(r'\n    (?:else )?if \(', body)
    for branch in branches:
        names = re.findall(r'name == "(\w+)"', branch.split('\n', 1)[0])
        if not names:
            continue
        for n in names:
            # Properties of the widget element itself (not of lines, radios...)
            tags[n] = reads(branch, 'elem')
            for (owner, item), variable in ITEM_VARIABLES.items():
                if owner == n:
                    tags[owner + '/' + item] = reads(branch, variable)
        # Histogram and DataLog share a branch; these reads only apply to the other one.
        if names == ['Histogram', 'DataLog']:
            tags['Histogram'] -= {'LineColor'}
            tags['DataLog'] -= {'BarWidth', 'Color'}
        # The old single-line PlotLines format reads the line properties from the widget
        if 'PlotLines' in names:
            tags['PlotLines'] |= reads(branch, 'e')
    return tags


def main():
    # WIDGET_SMOKE_BUILD=<dir> keeps the build there and reuses the third-party objects.
    keep = os.environ.get('WIDGET_SMOKE_BUILD')
    with tempfile.TemporaryDirectory(prefix='widget-smoke-') as directory:
        build = Path(keep or directory)
        build.mkdir(parents=True, exist_ok=True)
        # Main.h without SDL_syswm.h (windows.h), the SDL backends and the J-Link DLL header
        main_h = (ROOT / 'Main.h').read_text(encoding='utf-8-sig')
        for pattern in (r'#define main SDL_main\n', r'#include <SDL_syswm.h>\n',
                        r'#include <backends/[^>]+>\n', r'#include <JLinkARMDLL.h>\n'):
            main_h, count = re.subn(pattern, '', main_h)
            assert count >= 1, pattern
        main_h = main_h.replace('#endif // __MAIN_H__', '#include "msvc_shims.h"\n#endif // __MAIN_H__')
        (build / 'Main.h').write_text(main_h)
        (build / 'msvc_shims.h').write_text((HERE / 'msvc_shims.h').read_text())

        # Real helper functions from DataHelper.cpp (the rest of it needs the J-Link thread)
        helper = (ROOT / 'DataHelper.cpp').read_text(encoding='utf-8-sig')
        parts = ['#include "Main.h"', '#include "Editor/XMLSafety.h"', '#include <vector>',
                 'using namespace tinyxml2;']
        for signature in ('XMLElement* TryFindElement(', 'uint32_t AddressFromHex(',
                          'float average(', 'std::vector<float> smooth('):
            parts.append(function(helper, signature))
        (build / 'data_helpers.cpp').write_text('\n'.join(parts))

        compiler = os.environ.get('CXX', 'g++')
        flags = ['-std=c++17', '-g', '-O1', '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                 '-fno-sanitize-recover=undefined', '-include', str(HERE / 'smoke_assert.h')]
        includes = ['-I', str(build), '-I', str(ROOT), '-I', str(ROOT / 'include'),
                    '-I', str(ROOT / 'include/imgui'), '-I', str(ROOT / 'include/implot'),
                    '-I', str(ROOT / 'include/SDL2'), '-I', str(ROOT / 'include/ifqueue'),
                    '-I', str(ROOT / 'Widget'), '-I', str(ROOT / 'Editor')]
        sources = [HERE / 'widget_smoke.cpp', build / 'data_helpers.cpp',
                   ROOT / 'xml_window.cpp', ROOT / 'Editor/WidgetTemplateFactory.cpp',
                   ROOT / 'Widget/GlossyButtonWidget.cpp', ROOT / 'Widget/MemoryEditorWidget.cpp',
                   ROOT / 'Widget/WatchWidget.cpp',
                   ROOT / 'include/tinyxml2/tinyxml2.cpp',
                   ROOT / 'include/imgui/imgui.cpp', ROOT / 'include/imgui/imgui_draw.cpp',
                   ROOT / 'include/imgui/imgui_tables.cpp', ROOT / 'include/imgui/imgui_widgets.cpp',
                   ROOT / 'include/implot/implot.cpp', ROOT / 'include/implot/implot_items.cpp']
        # Compile in parallel, link once
        objects, jobs = [], []
        for source in sources:
            obj = build / (source.stem + '.o')
            objects.append(str(obj))
            if keep and 'include' in source.parts and obj.exists() and obj.stat().st_mtime > source.stat().st_mtime:
                continue
            jobs.append(subprocess.Popen([compiler, *flags, *includes, *(['-w'] if 'include' in source.parts else ['-Wall', '-Wno-format-truncation']),
                                          '-c', str(source), '-o', str(obj)]))
        if any(job.wait() for job in jobs):
            raise SystemExit('compilation failed')
        binary = build / 'widget_smoke'
        subprocess.run([compiler, *flags, *objects, '-o', str(binary)], check=True)
        env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
               'SMOKE_DIR': str(build), 'SMOKE_DEMO': str(ROOT / 'examples/demo.xml')}
        result = subprocess.run([str(binary), '--dump-schema'], check=True, env=env, capture_output=True, text=True)

        # The editor's table must list every property the loader reads
        schema = {}
        for line in result.stdout.splitlines():
            tag, _, props = line.partition(':')
            schema[tag] = set(props.split())
        loader = parser_tags()
        problems = []
        for tag, props in loader.items():
            if tag not in schema:
                problems.append(f'<{tag}> is loaded but missing from the editor table')
                continue
            missing = props - schema[tag]
            if missing:
                problems.append(f'<{tag}>: the editor does not offer {sorted(missing)}')
        for tag, props in schema.items():
            if tag not in loader:
                problems.append(f'<{tag}> is in the editor table but the loader does not know it')
                continue
            unused = props - loader[tag]
            if unused:
                problems.append(f'<{tag}>: the editor offers {sorted(unused)}, which the loader ignores')
        if problems:
            raise SystemExit('Editor table and loader disagree:\n  ' + '\n  '.join(problems))
        print(f'Editor table matches the loader ({len(loader)} widgets)')

        subprocess.run([str(binary)], check=True, env=env)


if __name__ == '__main__':
    main()
