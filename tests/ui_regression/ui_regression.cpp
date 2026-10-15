#include "ui_stubs.h"
#include "XMLSafety.h"
#include "editor_parsing_under_test.h"
#include "PlotTooltipWidget.h"
#include "PlotLinesWidget.h"
#include "GlossyDisplayWidget.h"
#include "TextFormattedWidget.h"
#include "TextFormattedFWidget.h"
#include "LabelTextWidget.h"
#include "SelectableTextWidget.h"
#include "InputFloatWidget.h"
#include "SliderFloatWidget.h"
#include "SliderIntWidget.h"
#include "HistogramWidget.h"
#include "DataLogWidget.h"
#include "VerticalGaugeWidget.h"
#include "MemoryViewWidget.h"

static void TestFormats() {
    using namespace ui_safety;
    char out[64];
    FormatScalar(out,sizeof out,"value=%04d %%",42);
    assert(std::string(out)=="value=0042 %");
    FormatScalar(out,sizeof out,"%08X",-1);
    assert(std::string(out)=="FFFFFFFF");
    FormatScalar(out,sizeof out,"%.2lf",1.25);
    assert(std::string(out)=="1.25");
    FormatScalar(out,sizeof out,"%.1f V",2.5f);
    assert(std::string(out)=="2.5 V");
    const char* bad[]={"%s","%n","%d %d","%*d","%1$d","%lld","%I64d","%999999999d","%","%.","%.1025d"};
    for(const char* f:bad) {
        assert(!IsScalarFormat(f,ScalarKind::Integer));
        FormatScalar(out,sizeof out,f,42);
        assert(std::string(out)==f);
    }
    assert(!IsScalarFormat("%d",ScalarKind::Floating));
    assert(!IsScalarFormat("%f",ScalarKind::Integer));
    assert(!IsScalarFormat("%Lf",ScalarKind::Floating));
    assert(!IsScalarFormat("%f %f",ScalarKind::Floating));
    assert(!IsScalarFormat(nullptr,ScalarKind::Floating));
    assert(std::string(ScalarFormatOrDefault("%n",ScalarKind::Floating))=="%.3f");
    char tiny[4]={'x','x','x','x'};
    FormatScalar(tiny,sizeof tiny,"%010d",42);
    assert(tiny[3]=='\0');
    FormatScalar(out,sizeof out,"literal %%",1);
    assert(std::string(out)=="literal %");
    MainContext ctx;
    rawValue=42;
    TextFormattedWidget malicious("%s %n",1);
    malicious.Draw(&ctx,nullptr);
    assert(drawnText=="%s %n");
}

static void TestXML() {
    using namespace tinyxml2;
    XMLDocument doc;
    assert(doc.Parse("<Window><Text/><Items><Line><Name/></Line></Items></Window>")==XML_SUCCESS);
    XMLElement* root=doc.FirstChildElement();
    assert(std::string(ui_safety::XmlChildText(root,"Text","Unnamed"))=="Unnamed");
    assert(std::string(ui_safety::XmlChildText(root,"Missing"))=="");
    assert(ui_safety::XmlChildText(nullptr,"Text",nullptr)==nullptr);
    XMLElement* line=root->FirstChildElement("Items")->FirstChildElement("Line");
    std::string name=ui_safety::XmlChildText(line,"Name");
    assert(name.empty());
    assert(ui_safety::ContainsXMLNode(&doc,line));
    root->DeleteChild(root->FirstChildElement("Items"));
    assert(!ui_safety::ContainsXMLNode(&doc,line));
    const XMLNode* stale=nullptr;
    {
        auto old=std::make_unique<XMLDocument>();
        old->Parse("<Old><Selection/></Old>");
        stale=old->FirstChildElement()->FirstChildElement();
    }
    assert(!ui_safety::ContainsXMLNode(&doc,stale));
    // tinyxml2's insertion APIs move existing nodes without freeing selections.
    doc.Clear();
    doc.Parse("<Root><A/><B/><C/></Root>");
    root=doc.FirstChildElement();
    XMLElement* a=root->FirstChildElement("A");
    XMLElement* b=root->FirstChildElement("B");
    XMLElement* c=root->FirstChildElement("C");
    root->InsertFirstChild(b);
    assert(root->FirstChildElement()==b && b->NextSiblingElement()==a);
    root->InsertAfterChild(c,b);
    assert(c->NextSiblingElement()==b && ui_safety::ContainsXMLNode(&doc,b));
}

static void TestParsing() {
    bool floating=false,boolean=false;
    assert(IsNumberString("2147483647",floating) && !floating);
    assert(IsNumberString("-2147483648",floating) && !floating);
    assert(!IsNumberString("2147483648",floating));
    assert(!IsNumberString("9999999999999999999999999999",floating));
    assert(!IsNumberString("nan",floating));
    assert(!IsNumberString("inf",floating));
    assert(!IsNumberString("1e9999",floating));
    assert(!IsNumberString("0xFFFFFFFF",floating));
    assert(IsNumberString("2.25e1  ",floating) && floating);
    assert(!IsNumberString("3junk",floating));
    assert(IsBooleanString("TrUe",boolean) && boolean);
    const char nonAscii[]={static_cast<char>(0xFF),'\0'};
    assert(!IsBooleanString(nonAscii,boolean));
    assert(!IsNumberString(nonAscii,floating));
    ImVec4 color;
    assert(ParseColor("#80FFAABB",color));
    assert(!ParseColor("#12zzzz",color));
    assert(!ParseColor("#1234567g",color));
}

static void TestTooltipBounds() {
    MainContext ctx;
    rawValue=7;
    PlotTooltipWidget large("large","%.1f",0,10,8,17,1,true,U08_DEC,false);
    for(int i=0;i<7;++i)large.Draw(&ctx,nullptr);
    // No address: prior code skipped cursor normalization by its write path.
    PlotTooltipWidget small("small","%.1f",0,10,2,17,0,true,U08_DEC,false);
    small.Draw(&ctx,nullptr);
    assert(ImGui::plotted.size()==2);
    small.Draw(&ctx,nullptr);
    PlotTooltipWidget invalid("invalid","%n",0,10,0,-1,0,true,U08_DEC,false);
    invalid.Draw(&ctx,nullptr);
    assert(ImGui::plotted.size()==1 && drawnText=="%n");
}

static void TestPlotHistory() {
    for(int capacity: {0,-1,INT_MIN,1,3}) {
        ScrollingData history(capacity);
        assert(history.MaxSize>=1);
        for(int i=0;i<10;++i)history.AddPoint(static_cast<float>(i),static_cast<float>(i));
        assert(history.Data.size()<=static_cast<std::size_t>(history.MaxSize));
        assert(history.Offset>=0 && history.Offset<history.MaxSize);
        history.Erase();
        assert(history.Data.empty() && history.Offset==0);
    }
    MainContext ctx;
    PlotLineConfig line;
    line.address=1;
    PlotLinesWidget graph("test",0,10,100,100,1,8,0,false,0,{line});
    graph.Draw(&ctx,nullptr);
}

// Plot points come from timestamped samples, thinned out to the plot frequency.
static void TestPlotSampling() {
    MainContext ctx;
    PlotLineConfig line;
    line.address=0x20000010; line.dataType=S16_DEC; line.littleEndian=true;
    line.color=IM_COL32(255,0,0,255);
    PlotLinesWidget graph("test",0,0,100,100,100,8,0,false,0,{line});
    timeNow=10.0;
    graph.Draw(&ctx,nullptr);
    for(int i=0;i<1000;++i) {            // one second at 1 kHz
        DataSample s{};
        s.time=10.0+i*0.001;
        const int16_t v=static_cast<int16_t>(-i);
        std::memcpy(s.bytes,&v,sizeof(v));
        pendingSamples.push_back(s);
    }
    timeNow=11.0;
    graph.Draw(&ctx,nullptr);
    assert(lastAddress==0x20000010);
    assert(ImPlot::lines.size()>=99 && ImPlot::lines.size()<=101);
    assert(ImPlot::lines.front().x==0.0f && ImPlot::lines.front().y==0.0f);
    for(std::size_t i=1;i<ImPlot::lines.size();++i) {
        assert(ImPlot::lines[i].x-ImPlot::lines[i-1].x>0.0099f);
        assert(ImPlot::lines[i].y<ImPlot::lines[i-1].y);
    }
    // IM_COL32 keeps red in the low byte: a red line must stay red.
    assert(ImPlot::lineColor.x==1.0f && ImPlot::lineColor.z==0.0f);
}

// DoneAddr: the flag is cleared only after a complete read of the arrays.
static void TestCaptureHandshake() {
    MainContext ctx;
    DataLogWidget::LineConfig logLine;
    logLine.baseAddr=0x100; logLine.dataType=U08_DEC;
    DataLogWidget log("log","%n",0,10,100,100,1,4,0,0x200,true,{logLine});
    for(int i=0;i<4;++i) ctx.IOMAP_Data[0x100+i]=static_cast<uint8_t>(i+1);
    rawValue=0; writes32=0; completedCycles=5;
    log.Draw(&ctx,nullptr);                    // flag clear: nothing taken
    assert(ImPlot::lines.size()==4 && ImPlot::lines[3].y==0.0f && writes32==0);
    rawValue=1;
    log.Draw(&ctx,nullptr);                    // flag seen: arrays requested
    completedCycles=6;
    log.Draw(&ctx,nullptr);                    // the cycle in progress may miss them
    assert(ImPlot::lines[3].y==0.0f && writes32==0);
    completedCycles=7;
    log.Draw(&ctx,nullptr);
    assert(ImPlot::lines[3].y==4.0f && writes32==1 && lastWrite32==0 && lastAddress==0x200);
    rawValue=0;
}

static void TestGlossyDisplay() {
    MainContext ctx;
    struct Case {int type;uint64_t value;bool hex;const char* expected;};
    const Case cases[]={
        {S08_DEC,static_cast<uint8_t>(-5),false,"-5"},{U08_DEC,42,false,"42"},
        {S16_DEC,static_cast<uint16_t>(-123),false,"-123"},{U16_DEC,1234,false,"1234"},
        {S32_DEC,static_cast<uint32_t>(-123456),false,"-123456"},{U32_DEC,4000000000u,false,"4000000000"},
        {S64_DEC,static_cast<uint64_t>(-1234567890123ll),false,"-1234567890123"},
        {U64_DEC,18446744073709551615ull,false,"18446744073709551615"},
        {U08_DEC,0xAB,true,"AB"},{U16_DEC,0xABCD,true,"ABCD"},
        {U32_DEC,0xABCDEF01,true,"ABCDEF01"},{U64_DEC,0xFEDCBA9876543210ull,true,"FEDCBA9876543210"}
    };
    for(const auto& c:cases) {
        rawValue=c.value;
        GlossyDisplayWidget widget("value",0x20001234,120,40,true,c.type,c.hex,0);
        widget.Draw(&ctx,nullptr);
        assert(drawnText==c.expected);
        assert(lastAddress==0x20001234);
    }
    rawValue=42;
    GlossyDisplayWidget padded("value",1,120,40,true,U16_DEC,false,4);
    padded.Draw(&ctx,nullptr);
    assert(drawnText=="0042");
    GlossyDisplayWidget negativeWidth("value",1,120,40,true,U16_DEC,false,INT_MIN);
    negativeWidth.Draw(&ctx,nullptr);
    assert(drawnText=="42");
    VerticalGaugeWidget gauge("address",0x20005678,0,100,120,300,true,U16_DEC);
    gauge.Draw(&ctx,nullptr);
    assert(lastAddress==0x20005678);
}

static void TestMappedMemoryBounds() {
    using ui_safety::IsMappedRange;
    assert(IsMappedRange(0x20000000,0x20000000,4096,4096));
    assert(IsMappedRange(0x20000FFF,0x20000000,1,4096));
    assert(!IsMappedRange(0x1FFFFFFF,0x20000000,1,4096));
    assert(!IsMappedRange(0x20000FFF,0x20000000,2,4096));
    assert(!IsMappedRange(0x20001000,0x20000000,1,4096));
    assert(IsMappedRange(UINT32_MAX,UINT32_MAX,1,4096));
    assert(!IsMappedRange(UINT32_MAX,UINT32_MAX,2,4096));
    MainContext ctx;
    ctx.mcu_memory_base=0x20000000;
    for(uint32_t address:{0u,0x1FFFFFFFu,0x20000FFFu,UINT32_MAX}) {
        DataLogWidget::LineConfig logLine;
        logLine.baseAddr=address;
        DataLogWidget log("log","%n",0,10,100,100,1,8,0,0,true,{logLine});
        log.Draw(&ctx,nullptr);
        HistogramWidget::LineConfig histogramLine;
        histogramLine.baseAddr=address;
        HistogramWidget histogram("histogram","%s",0,10,100,100,1,8,0,0,true,false,0.8f,{histogramLine});
        histogram.Draw(&ctx,nullptr);
        MemoryViewWidget memory("memory",address,8,0);
        memory.Draw(&ctx,nullptr);
    }
    for(int count:{0,-1,INT_MIN,INT_MAX}) {
        DataLogWidget log("log","%.1f",0,10,100,100,1,count,0,0,0,true,1,IM_COL32_WHITE);
        log.Draw(&ctx,nullptr);
        HistogramWidget histogram("histogram","%.1f",0,10,100,100,1,count,0,0,true,false,0.8f,
                                  0,1,IM_COL32_WHITE,FLOAT);
        histogram.Draw(&ctx,nullptr);
    }
}

int main() {
    TestFormats(); TestXML(); TestParsing(); TestTooltipBounds();
    TestPlotHistory(); TestPlotSampling(); TestCaptureHandshake(); TestGlossyDisplay();
    TestMappedMemoryBounds();
    std::puts("UI regression tests passed");
}
