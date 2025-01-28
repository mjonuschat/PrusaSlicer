** Dear ImGui is a bloat-free graphical user interface library for C++.**

For more information go to https://github.com/ocornut/imgui

THIS DIRECTORY CONTAINS THE imgui-1.91.7 5c1d2d1 SOURCE DISTRIBUTION.

### Changes from original code:

## 1)
> Method *void ImFont::RenderText() const* (in *imgui_draw.cpp*) modified to automatically change text color with the following code:

> ```
> #include "imconfig.h"
> ```

> ```
> ImU32 defaultCol = col;
> ImU32 highlighCol = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
> // if text is started with ColorMarkerHovered symbol, we should use another color for a highlighting
> if (*s == ImGui::ColorMarkerHovered) {
>      highlighCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
>      s += 1;
>  }
> ```

> and

> ```
> if (*s == ImGui::ColorMarkerStart) {
>     col = highlighCol;
>     s += 1;
> }
> else if (*s == ImGui::ColorMarkerEnd) {
>     col = defaultCol;
>     s += 1;
>     if (s == text_end)
>         break;
> }
> ```

> by using the definitions of *const char ColorMarkerXXXX* added into *imconfig.h*:

> ```
> // Special ASCII character is used here as markup symbols for tokens to be highlighted as a for hovered item
> const char ColorMarkerHovered   = 0x1; // STX
>
> // Special ASCII characters STX and ETX are used here as markup symbols for tokens to be highlighted.
> const char ColorMarkerStart = 0x2; // STX
> const char ColorMarkerEnd   = 0x3; // ETX
> ```

## 2)
> Method *ImFontGlyph* ImFont::FindGlyph(ImWchar c) const* (in *imgui_draw.cpp*) modified to automatically add missing glyphs into imgui atlas with the following code:
> ```
>const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const
>{
>    // PrusaSlicer extension: call the following function whenever the fallback is needed.
>    // The goal is to not modify ImGui code too much.
>    void imgui_rendered_fallback_glyph(ImWchar c);
>
>    if (c >= (size_t)IndexLookup.Size) {
>        imgui_rendered_fallback_glyph(c);
>        return FallbackGlyph;
>    }
>    const ImWchar i = IndexLookup.Data[c];
>    if (i == (ImWchar)-1) {
>       imgui_rendered_fallback_glyph(c);
>        return FallbackGlyph;
>    }
>    return &Glyphs.Data[i];
>}
> ```


## 3)
> Icons added into FontAtlas in *imconfig.h*
> ```
>    // Special ASCII characters are used here as an ikons markers
>    const wchar_t PrintIconMarker = 0x4;
>    const wchar_t PrinterIconMarker = 0x5;
>    const wchar_t PrinterSlaIconMarker = 0x6;
>    const wchar_t FilamentIconMarker = 0x7;
>    const wchar_t MaterialIconMarker = 0x8;
>    const wchar_t CloseNotifButton = 0xB;
>    const wchar_t CloseNotifHoverButton = 0xC;
>    const wchar_t MinimalizeButton = 0xE;
>    const wchar_t MinimalizeHoverButton = 0xF;
>    const wchar_t WarningMarker = 0x10;
>    const wchar_t ErrorMarker = 0x11;
>    const wchar_t EjectButton = 0x12;
>    const wchar_t EjectHoverButton = 0x13;
>    const wchar_t CancelButton = 0x14;
>    const wchar_t CancelHoverButton = 0x15;
>//    const wchar_t VarLayerHeightMarker     = 0x16;
>    const wchar_t RevertButton = 0x16;
>
>    const wchar_t RightArrowButton = 0x18;
>    const wchar_t RightArrowHoverButton = 0x19;
>    const wchar_t PreferencesButton = 0x1A;
>    const wchar_t PreferencesHoverButton = 0x1B;
>//    const wchar_t SinkingObjectMarker      = 0x1C;
>//    const wchar_t CustomSupportsMarker     = 0x1D;
>//    const wchar_t CustomSeamMarker         = 0x1E;
>//    const wchar_t MmuSegmentationMarker    = 0x1F;
>    const wchar_t PlugMarker = 0x1C;
>    const wchar_t DowelMarker = 0x1D;
>    const wchar_t SnapMarker = 0x1E;
>    const wchar_t HorizontalHide = 0xB4;
>    const wchar_t HorizontalShow = 0xB6;
>    // Do not forget use following letters only in wstring
>    const wchar_t DocumentationButton = 0x2600;
>    const wchar_t DocumentationHoverButton = 0x2601;
>    const wchar_t ClippyMarker = 0x2602;
>    const wchar_t InfoMarker = 0x2603;
>    const wchar_t SliderFloatEditBtnIcon = 0x2604;
>    const wchar_t SliderFloatEditBtnPressedIcon = 0x2605;
>    const wchar_t ClipboardBtnIcon = 0x2606;
>    const wchar_t PlayButton = 0x2618;
>    const wchar_t PlayHoverButton = 0x2619;
>    const wchar_t PauseButton = 0x261A;
>    const wchar_t PauseHoverButton = 0x261B;
>    const wchar_t OpenButton = 0x261C;
>    const wchar_t OpenHoverButton = 0x261D;
>    const wchar_t SlaViewOriginal = 0x261E;
>    const wchar_t SlaViewProcessed = 0x261F;
>
>    const wchar_t LegendTravel = 0x2701;
>    const wchar_t LegendWipe = 0x2702;
>    const wchar_t LegendRetract = 0x2703;
>    const wchar_t LegendDeretract = 0x2704;
>    const wchar_t LegendSeams = 0x2705;
>    const wchar_t LegendToolChanges = 0x2706;
>    const wchar_t LegendColorChanges = 0x2707;
>    const wchar_t LegendPausePrints = 0x2708;
>    const wchar_t LegendCustomGCodes = 0x2709;
>    const wchar_t LegendCOG = 0x2710;
>    const wchar_t LegendShells = 0x2711;
>    const wchar_t LegendToolMarker = 0x2712;
>    const wchar_t WarningMarkerSmall = 0x2713;
>    const wchar_t ExpandBtn = 0x2714;
>    const wchar_t InfoMarkerSmall = 0x2716;
>    const wchar_t CollapseBtn = 0x2715;
>
>    // icons for double slider (middle size icons)
>    const wchar_t Lock = 0x2801;
>    const wchar_t LockHovered = 0x2802;
>    const wchar_t Unlock = 0x2803;
>    const wchar_t UnlockHovered = 0x2804;
>    const wchar_t DSRevert = 0x2805;
>    const wchar_t DSRevertHovered = 0x2806;
>    const wchar_t DSSettings = 0x2807;
>    const wchar_t DSSettingsHovered = 0x2808;
>    // icons for double slider (small size icons)
>    const wchar_t ErrorTick = 0x2809;
>    const wchar_t ErrorTickHovered = 0x280A;
>    const wchar_t PausePrint = 0x280B;
>    const wchar_t PausePrintHovered = 0x280C;
>    const wchar_t EditGCode = 0x280D;
>    const wchar_t EditGCodeHovered = 0x280E;
>    const wchar_t RemoveTick = 0x280F;
>    const wchar_t RemoveTickHovered = 0x2810;
>
>    // icon for multiple beds
>    const wchar_t SliceAllBtnIcon = 0x2811;
>    const wchar_t PrintIdle = 0x2812;
>    const wchar_t PrintRunning = 0x2813;
>    const wchar_t PrintFinished = 0x2814;
>    const wchar_t WarningMarkerDisabled = 0x2815;
> ```

## 4)
> *imstb_truetype.h* modification:
> Hot fix for open symbolic fonts on windows
> Add case *STBTT_MS_EID_SYMBOL* to switch in file *imstb_truetype.h* on line 1480.

## 5)
> Added functions:
> ```
>    IMGUI_API bool ColorEdit4(const char* label, float col[4], const char* current_label, const char* original_label, ImGuiColorEditFlags flags = 0);
>    IMGUI_API bool ColorPicker4(const char* label, float col[4], const char* current_label, const char* original_label, ImGuiColorEditFlags flags = 0, const float* ref_col = NULL);
> ```
> into *imgui.h* and *imgui_widgets.cpp* to allow to localize the labels contained into the color picker dialog
 
## 6)
> Enabled #define IMGUI_DEFINE_MATH_OPERATORS into *imconfig.h*:
