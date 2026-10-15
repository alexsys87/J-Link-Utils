#pragma once
#ifndef TASKLET_UI_REGRESSION_STUBS_H
#define TASKLET_UI_REGRESSION_STUBS_H
// Headless test doubles: no SDL, J-Link device, or renderer is required.
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ScalarFormat.h"
#include "MemoryBounds.h"

#define IOMAP_SIZE 4096
#define sscanf_s sscanf

constexpr int MAX_BUFF = 64, BUFF_SIZE = 4096, MAX_ITEM_LEN = 64;
enum { BOOLEAN, S08_DEC, U08_DEC, S16_DEC, U16_DEC, S32_DEC, U32_DEC,
       FLOAT, FLOAT_INV, S64_DEC, U64_DEC };
using ImU32 = unsigned int;
#define IM_COL32(R,G,B,A) ((ImU32)(R) | ((ImU32)(G)<<8) | ((ImU32)(B)<<16) | ((ImU32)(A)<<24))
#define IM_COL32_WHITE 0xFFFFFFFFu
struct ImVec2 { float x, y; ImVec2(float a=0, float b=0):x(a),y(b){} };
struct ImVec4 { float x,y,z,w; ImVec4(float a=0,float b=0,float c=0,float d=0):x(a),y(b),z(c),w(d){} };
template<class T> using ImVector = std::vector<T>;
struct ImRect { ImVec2 Min, Max; ImRect(ImVec2 a,ImVec2 b):Min(a),Max(b){} };
inline std::string drawnText;
struct ImDrawList {
    template<class... T> void AddRectFilled(T...){}
    template<class... T> void AddRect(T...){}
    template<class... T> void AddRectFilledMultiColor(T...){}
    template<class... T> void AddLine(T...){}
    void AddText(ImVec2,ImU32,const char* text){ drawnText=text; }
};
struct ImGuiWindow {
    bool SkipItems=false;
    struct { ImVec2 CursorPos; } DC;
    ImDrawList draw;
    ImDrawList* DrawList=&draw;
    float FontWindowScale=1;
};
constexpr int ImPlotFlags_Crosshairs=0, ImAxis_X1=0, ImAxis_Y1=1;
constexpr int ImGuiCond_Always=0, ImGuiCond_Once=1, ImGuiInputTextFlags_EnterReturnsTrue=0;
enum {ImGuiCol_FrameBg,ImGuiCol_Border,ImGuiCol_Text,ImGuiCol_TextDisabled,ImGuiCol_SliderGrabActive};
struct ImGuiStyle {float FrameRounding=0,FrameBorderSize=0;ImVec2 ItemSpacing{4,4};};
constexpr int ImGuiWindowFlags_HorizontalScrollbar=0, ImGuiInputTextFlags_CharsHexadecimal=0;
inline float ImSaturate(float v){return std::clamp(v,0.0f,1.0f);}
struct ImPlotSpec { int Offset=0, Stride=0; ImVec4 LineColor; float LineWeight=1.0f; };
struct ImGuiIO {float DeltaTime=0.1f;bool MouseDown[5]={};};
#define IM_DRAWLIST_TEX_LINES_WIDTH_MAX 32
namespace ImGui {
inline ImVec4 ColorConvertU32ToFloat4(ImU32 c){return ImVec4((c&0xFF)/255.0f,((c>>8)&0xFF)/255.0f,((c>>16)&0xFF)/255.0f,((c>>24)&0xFF)/255.0f);}
inline bool hovered=true;
inline int plotCount=0;
inline std::vector<float> plotted;
inline bool IsItemHovered(){return hovered;}
inline bool IsItemActive(){return false;}
inline void BeginTooltip(){}
inline void EndTooltip(){}
inline void Spacing(){}
template<class... T> void Text(const char* format,T... args){char b[256];std::snprintf(b,sizeof b,format,args...);drawnText=b;}
template<class... T> void TextColored(ImVec4,const char* format,T... args){Text(format,args...);}
inline void LabelText(const char*,const char*,const char* text){drawnText=text;}
inline bool Selectable(const char* text,bool*){drawnText=text;return false;}
inline void PlotLines(const char* label,const float* values,int count,int,const char*,float,float,ImVec2){
    drawnText=label; ++plotCount; plotted.assign(values,values+count);
}
inline ImGuiWindow* GetCurrentWindow(){static ImGuiWindow w;return &w;}
inline ImGuiStyle& GetStyle(){static ImGuiStyle s;return s;}
inline ImVec2 GetCursorScreenPos(){return ImVec2();}
inline ImU32 GetColorU32(int){return 0;}
inline void ItemSize(ImRect){}
inline bool ItemAdd(ImRect,unsigned){return true;}
inline unsigned GetID(const char*){return 0;}
inline float GetFontSize(){return 12;}
inline void SetWindowFontScale(float){}
inline ImVec2 CalcTextSize(const char*){return ImVec2(20,10);}
template<class T> void PushID(T){}
inline void PopID(){}
inline double GetTime(){return 1;}
inline ImGuiIO& GetIO(){static ImGuiIO io;return io;}
inline bool CollapsingHeader(const char*){return false;}
inline void PushItemWidth(float){}
inline void PopItemWidth(){}
inline bool Checkbox(const char*,bool*){return false;}
inline void SameLine(float=0,float=-1){}
inline void SetCursorPosX(float){}
inline void Separator(){}
inline bool BeginChild(const char*,ImVec2,bool,int){return true;}
inline void EndChild(){}
template<class... T> bool InputText(T...){return false;}
inline void SetNextItemWidth(float){}
template<class... T> bool InputFloat(T...){return false;}
template<class... T> bool DragFloat(T...){return false;}
template<class... T> bool SliderFloat(T...){return false;}
template<class... T> bool SliderInt(T...){return false;}
inline ImVec2 GetContentRegionAvail(){return ImVec2(100,100);}
}
namespace ImPlot {
inline bool BeginPlot(const char*,ImVec2,int){return true;}
inline void SetupAxes(const char*,const char*){}
inline void SetupAxisLimits(int,double,double,int){}
// Records the last line as (x, y) points in drawing order.
inline std::vector<ImVec2> lines;
inline ImVec4 lineColor;
inline void PlotLine(const char*,const float* xs,const float* ys,int count,const ImPlotSpec& spec){
    lines.clear(); lineColor=spec.LineColor;
    const int stride=spec.Stride?spec.Stride:static_cast<int>(sizeof(float));
    for(int i=0;i<count;++i){
        const int k=(spec.Offset+i)%count;
        lines.push_back(ImVec2(*reinterpret_cast<const float*>(reinterpret_cast<const char*>(xs)+k*stride),
                               *reinterpret_cast<const float*>(reinterpret_cast<const char*>(ys)+k*stride)));
    }
}
template<class... T> void PlotBars(T...){}
inline void EndPlot(){}
}
struct MainContext {
    bool StartComunication=true;
    uint32_t mcu_memory_base=0;
    void* Datalock=nullptr;
    uint8_t IOMAP_Data[4096]={};
};
inline void SDL_LockMutex(void*){}
inline void SDL_UnlockMutex(void*){}
struct FT32 {union {uint32_t hex;float real;};};
inline uint64_t rawValue=0;
inline uint32_t lastAddress=0;
// Last write through the typed stubs: raw value and size in bytes.
inline uint64_t lastWriteRaw=0;
inline int lastWriteSize=0;
inline uint8_t DataReadWriteU08(MainContext*,void*,void* v,uint32_t* a){
    lastAddress=*a;
    if(v){lastWriteRaw=*static_cast<uint8_t*>(v);lastWriteSize=1;}
    return static_cast<uint8_t>(rawValue);
}
inline uint16_t DataReadWriteU16(MainContext*,void*,void* v,uint32_t* a){
    lastAddress=*a;
    if(v){lastWriteRaw=*static_cast<uint16_t*>(v);lastWriteSize=2;}
    return static_cast<uint16_t>(rawValue);
}
inline uint32_t lastWrite32=0;
inline int writes32=0;
inline uint32_t DataReadWriteU32(MainContext*,void*,void* v,uint32_t* a){
    lastAddress=*a;
    if(v){lastWrite32=*static_cast<uint32_t*>(v);++writes32;lastWriteRaw=lastWrite32;lastWriteSize=4;}
    return static_cast<uint32_t>(rawValue);
}
struct DataSample { double time; uint8_t bytes[8]; };
inline std::vector<DataSample> pendingSamples;  // returned by the next DataReadSamples
inline size_t DataReadSamples(MainContext*,const void*,uint32_t a,uint16_t,std::vector<DataSample>& out){
    lastAddress=a; out=pendingSamples; pendingSamples.clear(); return out.size();
}
inline uint32_t completedCycles=0;
inline uint32_t DataCompletedCycles(){return completedCycles;}
inline double timeNow=0;
inline double DataTimeNow(){return timeNow;}
inline uint64_t DataReadWriteU64(MainContext*,void*,void* v,uint32_t* a){
    lastAddress=*a;
    if(v){lastWriteRaw=*static_cast<uint64_t*>(v);lastWriteSize=8;}
    return rawValue;
}
inline uint16_t DataReadWriteArray(MainContext*,void*,void*,uint32_t*,uint16_t size){return size;}
inline bool DataReadWriteBoolean(MainContext*,void*,void*,uint32_t* a){lastAddress=*a;return rawValue!=0;}
inline float DataReadWriteFT32(MainContext*,void*,void*,uint32_t* a){lastAddress=*a;uint32_t r=static_cast<uint32_t>(rawValue);float f;std::memcpy(&f,&r,4);return f;}
inline uint16_t Swap16(uint16_t v){return (v>>8)|(v<<8);}
inline uint32_t SwapLeToBe32(uint32_t v){return __builtin_bswap32(v);}
inline uint32_t SwapBeToLe32(uint32_t v){return SwapLeToBe32(v);}
inline uint64_t SwapLeToBe64(uint64_t v){return __builtin_bswap64(v);}
inline float average(const std::vector<float>& v){float sum=0;for(float x:v)sum+=x;return v.empty()?0:sum/v.size();}
inline std::vector<float> smooth(const std::vector<float>& v,int){return v;}
inline float VirtualBuffer[MAX_BUFF][BUFF_SIZE]={};
struct ColorRGBA {float R,G,B,A;};
inline ColorRGBA ColorConverter(int){return {1,1,1,1};}
#include "TypedValue.h"
class UIWidget {public:virtual ~UIWidget()=default;virtual void Draw(MainContext*,uint32_t* = nullptr)=0;};
class LeafWidget:public UIWidget {protected:std::string text;public:LeafWidget(const char* s=""):text(s?s:""){} };
#endif
