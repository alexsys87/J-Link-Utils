// WidgetTemplateFactory.h
#pragma once

#include <tinyxml2/tinyxml2.h>
#include <functional>
#include <map>
#include <string>

namespace WidgetTemplateFactory {

    using Creator = std::function<tinyxml2::XMLElement* (tinyxml2::XMLDocument* doc)>;

    // ----- Функции создания корневых виджетов -----
    tinyxml2::XMLElement* CreateNewWindow(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateText(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTextColored(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateButton(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateCheckbox(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGroup(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTreeNode(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSameLine(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSpacing(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSeparator(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDummy(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGrid(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreatePlotLines(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTextFormatted(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTextFormattedF(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateAlignText(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateNewLine(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLabelText(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateRadioButton(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTooltip(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreatePlotTooltip(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateComboBox(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateInputInt(tinyxml2::XMLDocument* doc);   // соответствует тегу "ImputInt"
    tinyxml2::XMLElement* CreateInputFloat(tinyxml2::XMLDocument* doc); // соответствует тегу "ImputFloat"
    tinyxml2::XMLElement* CreateFixedWidth(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSetCursorPos(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateIndent(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHistogram(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDataLog(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateProgressBar(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSliderFloat(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSliderInt(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTabBar(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTab(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateColumn(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSelectableText(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateChildWindow(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHeader(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreatePopup(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHintBox(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHelpMarker(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateWatch(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSevenSegment(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMechanicalSwitch2(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGauge(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGlossyDisplay(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLCD_HD44780(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateVerticalGauge(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateNeumorphicKnob(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateStyledKnob(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateCustomVerticalSlider(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateBitControl8Bit(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateBitControl(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateRotarySwitch(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLedIndicator(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateImage(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateRPMIndicator(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateKeypad(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateEqualizer(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateTank(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateVUMeter(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSegmentButton(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMemoryView(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGif(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGlossyButton(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMemoryEditor(tinyxml2::XMLDocument* doc);

    // ----- Функции создания простых дочерних элементов (параметры) -----
    tinyxml2::XMLElement* CreateTextElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateColorElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateWidthElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHeightElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateAddressElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateAddrBoolElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMinElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMaxElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateStepElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateFastElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateFormatElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateUnitElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateRadiusElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDigitsElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateModeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateValueElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateScaleElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateScaleXElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateScaleYElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateBarWidthElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDataTypeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateFreqElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSizeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSmoothElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateMaxSizeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateAutoSizeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateBufferIdElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLineColorElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLineWeightElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateBandWidthElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateAddressStepElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateOrientationElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateShowPeakElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLoopElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSpeedElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateReadOnlyElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateShowAsciiElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateShowHexIIElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateShowOptionsElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateShowPreviewElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateGreyZeroElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateUpperHexElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateUseExternalElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateUseCacheElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDecimalPlacesElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreatePixelSizeElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateUseFixedElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDoneAddrElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLine1AddressElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLine1TextElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLine2AddressElement(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateLine2TextElement(tinyxml2::XMLDocument* doc);

    // ----- Функции создания вложенных элементов (для контейнеров) -----
    tinyxml2::XMLElement* CreateComboItem(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateRadioItem(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateSegmentItem(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateZoneItem(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateDataLogLine(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreateHistogramLine(tinyxml2::XMLDocument* doc);
    tinyxml2::XMLElement* CreatePlotLinesLine(tinyxml2::XMLDocument* doc);

    // ----- Карта всех корневых виджетов (по имени) -----
    const std::map<std::string, Creator>& GetRootCreators();

    // ----- Карта допустимых дочерних элементов для конкретных родителей -----
    using ChildCreatorMap = std::map<std::string, Creator>;
    const std::map<std::string, ChildCreatorMap>& GetChildCreators();

    // Удобная функция для создания элемента по имени типа (использует корневую карту)
    inline tinyxml2::XMLElement* Create(const std::string& type, tinyxml2::XMLDocument* doc) {
        auto it = GetRootCreators().find(type);
        if (it != GetRootCreators().end()) {
            return it->second(doc);
        }
        return nullptr;
    }

} // namespace WidgetTemplateFactory