#!/usr/bin/env python3
"""Exercise actual main-window persistence helpers with an SDL boundary shim."""
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

helpers = function('static void CaptureMainWindowSettings(') + function('static void FitMainWindowToDisplay(')
menu = function('void ShowMenuWindow(')
assert 'SDL_SetWindowAlwaysOnTop(ctx->Wimdow, ctx->TopMost ? SDL_TRUE : SDL_FALSE);' in menu
assert 'SaveDashboardWindowVisibility();' in menu
assert 'ImGui::GetIO().IniFilename = imguiIniPath.c_str();' in source
assert 'ApplicationFile("window.ini")' in source
assert 'LoadStyleFromXML("custom.style.xml"' not in source
assert 'SaveStyleToXML("custom.style.xml"' not in source
assert 'prefs::Error()' in source and 'Retry saving settings' in source

test = r'''
#include <algorithm>
#include <cassert>
#include <cstdio>
using Uint32 = unsigned;
constexpr unsigned SDL_WINDOW_MINIMIZED=1, SDL_WINDOW_MAXIMIZED=2;
struct SDL_Rect { int x,y,w,h; };
struct SDL_Window { unsigned flags=0; int x=25,y=35,w=800,h=600; };
struct MainContext {
 SDL_Window* Wimdow=nullptr;
 int Win_x=0,Win_y=0,Win_width=800,Win_height=600;
 bool Win_position_saved=false,Win_maximized=false;
};
static int monitorCount=2;
Uint32 SDL_GetWindowFlags(SDL_Window* w){return w->flags;}
void SDL_GetWindowPosition(SDL_Window* w,int* x,int* y){*x=w->x;*y=w->y;}
void SDL_GetWindowSize(SDL_Window* w,int* x,int* y){*x=w->w;*y=w->h;}
int SDL_GetRectDisplayIndex(const SDL_Rect* r){
 if(r->x>=1280 && r->x<2560 && monitorCount>1)return 1;
 if(r->x>=0 && r->x<1280)return 0;
 return -1;
}
int SDL_GetDisplayUsableBounds(int display, SDL_Rect* r){
 if(display>=monitorCount)return -1;
 *r=SDL_Rect{display*1280,0,1280,720};return 0;
}
'''
test += helpers
test += r'''
int main(){
 MainContext c;CaptureMainWindowSettings(&c); // null is harmless before startup
 SDL_Window w;c.Wimdow=&w;CaptureMainWindowSettings(&c);
 assert(c.Win_position_saved && c.Win_x==25 && c.Win_y==35);
 assert(c.Win_width==800 && c.Win_height==600 && !c.Win_maximized);
 w.flags=SDL_WINDOW_MAXIMIZED;w.x=0;w.y=0;w.w=1280;w.h=720;
 CaptureMainWindowSettings(&c);
 assert(c.Win_maximized && c.Win_width==800 && c.Win_height==600 && c.Win_x==25);
 w.flags=SDL_WINDOW_MINIMIZED;CaptureMainWindowSettings(&c);
 assert(c.Win_maximized && c.Win_width==800); // minimize does not lose restore/max state
 w.flags=0;w.x=1340;w.y=20;w.w=900;w.h=650;CaptureMainWindowSettings(&c);
 assert(!c.Win_maximized && c.Win_x==1340 && c.Win_width==900);
 FitMainWindowToDisplay(&c);assert(c.Win_x==1340 && c.Win_y==20);
 monitorCount=1;FitMainWindowToDisplay(&c);
 assert(c.Win_x==380 && c.Win_y==20); // missing second display: rectangle is visible
 c.Win_x=-50000;c.Win_y=50000;c.Win_width=9000;c.Win_height=9000;
 FitMainWindowToDisplay(&c);
 assert(c.Win_x==0 && c.Win_y==0 && c.Win_width==1280 && c.Win_height==720);
 puts("Window persistence: normal/maximized/minimized geometry and missing-monitor recovery passed");
}
'''
with tempfile.TemporaryDirectory(prefix='settings-window-') as directory:
    build = Path(directory)
    (build/'test.cpp').write_text(test)
    binary = build/'test'
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++17','-O0','-g1',
                    '-fsanitize=address,undefined','-fno-pie','-no-pie',
                    str(build/'test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
