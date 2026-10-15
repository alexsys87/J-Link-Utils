#!/usr/bin/env python3
"""Exercise production preference identity/XML/restore functions without SDL/Windows.

Only the surrounding GUI classes are replaced with minimal test shells. Function
bodies are extracted verbatim from production sources; real tinyxml2 and the real
Preferences.h store handle configuration parsing and settings reloads.
Run: python3 tests/preferences_widgets/run.py
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def main():
    xml = (ROOT / 'xml_window.cpp').read_text()
    watch = (ROOT / 'Widget/WatchWidget.cpp').read_text()
    header = (ROOT / 'Widget/WatchWidget.h').read_text()
    plot = (ROOT / 'Widget/PlotLinesWidget.h').read_text()
    row = function(header, 'struct WatchRow {').rstrip() + ';'
    fragments = ['''
#include "Preferences.h"
#include "Editor/XMLSafety.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
using namespace tinyxml2;
using ui_safety::XmlChildText;
constexpr int MAX_PATH = 4096;
constexpr int BOOLEAN = 0;
template<size_t N> void strncpy_s(char (&to)[N], const char* from, size_t count) {
    std::strncpy(to, from, std::min(count, N - 1)); to[N - 1] = 0;
}
''']
    for signature in ('static const char* ReadText(', 'static std::string PreferenceIdentityPart(',
                      'static std::string PreferenceElementSignature(', 'static std::string PreferenceElementIdentity('):
        fragments.append(function(xml, signature))
    fragments.append('''
class WatchWidget {
public:
    static const int MAX_ROWS = 128, MAX_STR_LEN = 128;
''' + row + '''
    static std::vector<WatchRow> s_rows;
    static char s_xmlPath[MAX_PATH];
    static void LoadFromXML(const char*);
    static void SaveToXML(const char*);
    static std::string RowPreferenceIdentity(const WatchRow&);
    static void LoadRowPreferences(WatchRow&);
    static void SaveRowPreferences(const WatchRow&);
};
std::vector<WatchWidget::WatchRow> WatchWidget::s_rows;
char WatchWidget::s_xmlPath[MAX_PATH];
''')
    for signature in ('static std::string WatchIdentityPart(', 'static std::string NewWatchPreferenceId(',
                      'std::string WatchWidget::RowPreferenceIdentity(', 'void WatchWidget::LoadRowPreferences(',
                      'void WatchWidget::SaveRowPreferences(', 'void WatchWidget::LoadFromXML(',
                      'void WatchWidget::SaveToXML('):
        fragments.append(function(watch, signature))
    fragments.append('''
struct PlotRestore {
    std::string m_preferenceKey;
    static constexpr float kMaxFreq = 1000.0f;
    float freq = 3;
    struct { float timeWindow = 10; bool pause_check = true, paused = false; } state;
    int resized = 0;
    void ResizeHistory() { ++resized; }
''' + function(plot, 'void SetPreferenceKey(') + '};\n')
    # The visibility writer is also production code; no GUI is needed to test it.
    fragments.append('''
struct WindowData { bool show; };
std::vector<WindowData> g_Windows;
std::vector<std::string> g_WindowPreferenceKeys;
''')
    fragments.append(function(xml, 'void SaveDashboardWindowVisibility() {'))
    fragments.append((Path(__file__).parent / 'regression.cpp').read_text())
    with tempfile.TemporaryDirectory(prefix='preferences-widgets-') as directory:
        build = Path(directory)
        source = build / 'test.cpp'
        source.write_text('\n'.join(fragments))
        binary = build / 'preferences_widgets'
        subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-O1', '-g',
                        '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                        '-I', str(ROOT), '-I', str(ROOT / 'include'), str(source),
                        str(ROOT / 'include/tinyxml2/tinyxml2.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary), str(build)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1'})


if __name__ == '__main__':
    main()
