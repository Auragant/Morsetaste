// SPDX-License-Identifier: GPL-3.0-only
// LLM-assisted implementation; see LICENSE and DISCLAIMER.md.
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <algorithm>
#include <cwchar>
#include <memory>
#include <vector>
#include "appearance.hpp"

namespace morse {
inline bool systemDarkMode() {
    DWORD value = 1, size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS && value == 0;
}
inline bool highContrastMode() {
    HIGHCONTRASTW contrast{}; contrast.cbSize = sizeof(contrast);
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
           (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
}
inline ThemePalette systemPalette() {
    const auto background = GetSysColor(COLOR_WINDOW), text = GetSysColor(COLOR_WINDOWTEXT);
    const auto selection = GetSysColor(COLOR_HIGHLIGHT);
    return {background, text, text, text, text, GetSysColor(COLOR_WINDOWFRAME),
            text, background, background, background, GetSysColor(COLOR_BTNFACE),
            selection, selection, GetSysColor(COLOR_HIGHLIGHTTEXT),
            (5 * GetGValue(background) + 2 * GetRValue(background) + GetBValue(background)) < 8 * 128};
}

// Customise drawing only. The original classes retain input handling, combo
// selection/type-ahead, checkbox automation, edit caret and accessibility.
class UiTheme {
    enum class Kind { Button, Combo, ComboList, Edit, Spin };
    struct Entry {
        UiTheme* owner;
        HWND window;
        Kind kind;
        bool hot = false;
        int hotPart = 0, pressedPart = 0;
    };
    ThemePalette palette_ = paletteFor(ThemeId::Light, false, false);
    bool highContrast_ = false;
    HBRUSH backgroundBrush_ = nullptr, controlBrush_ = nullptr, surfaceBrush_ = nullptr;
    std::vector<std::unique_ptr<Entry>> entries_;
    unsigned callbackDepth_ = 0;
    static constexpr UINT_PTR subclassId = 0x4d425448;
    static constexpr UINT_PTR scrollbarTimer = 0x4d425453;
    static UINT controlDpi(HWND window) {
        UINT dpi = GetDpiForWindow(window);
        HWND fontWindow = window;
        wchar_t name[64]{}; GetClassNameW(window,name,64);
        if (_wcsicmp(name,UPDOWN_CLASSW) == 0) {
            const auto buddy = reinterpret_cast<HWND>(SendMessageW(window,UDM_GETBUDDY,0,0));
            if (buddy) fontWindow = buddy;
        }
        const auto selected = reinterpret_cast<HFONT>(SendMessageW(fontWindow,WM_GETFONT,0,0));
        LOGFONTW logical{};
        // All interactive controls use the application's 14px base font. Using
        // its live size also makes simulated-DPI render tests faithful, and
        // updates the spinner whose font is supplied by its edit buddy.
        if (selected && GetObjectW(selected,sizeof(logical),&logical) && logical.lfHeight != 0)
            dpi = UINT(MulDiv(std::abs(logical.lfHeight),96,14));
        return dpi ? dpi : 96;
    }
    static int scaled(HWND window, int value) { return MulDiv(value, int(controlDpi(window)), 96); }
    static void fill(HDC dc, const RECT& rectangle, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color); FillRect(dc, &rectangle, brush); DeleteObject(brush);
    }
    static void outline(HDC dc, const RECT& rectangle, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color); FrameRect(dc, &rectangle, brush); DeleteObject(brush);
    }
    static void triangle(HDC dc, int x, int y, int size, bool up, COLORREF color) {
        const POINT points[3]{{x-size,y+(up ? size/2 : -size/2)},
                              {x+size,y+(up ? size/2 : -size/2)},
                              {x,y+(up ? -size/2 : size/2)}};
        HPEN pen = CreatePen(PS_SOLID, 1, color); HBRUSH brush = CreateSolidBrush(color);
        HGDIOBJ oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, brush);
        Polygon(dc, points, 3);
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(brush); DeleteObject(pen);
    }
    static std::wstring label(HWND window) {
        const int length = GetWindowTextLengthW(window);
        if (length < 0 || length > 4096) return {};
        std::wstring value(std::size_t(length) + 1, L'\0');
        GetWindowTextW(window, value.data(), length + 1); value.resize(std::wcslen(value.c_str()));
        return value;
    }
    static std::wstring comboLabel(HWND window, int item) {
        if (item < 0) return {};
        const auto length = SendMessageW(window, CB_GETLBTEXTLEN, WPARAM(item), 0);
        if (length < 0 || length > 4096) return {};
        std::wstring value(std::size_t(length) + 1, L'\0');
        if (SendMessageW(window, CB_GETLBTEXT, WPARAM(item), reinterpret_cast<LPARAM>(value.data())) == CB_ERR) return {};
        value.resize(std::size_t(length)); return value;
    }
    static void font(HWND window, HDC dc) {
        const auto selected = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
        SelectObject(dc, selected ? selected : GetStockObject(DEFAULT_GUI_FONT));
    }
    static bool showFocus(HWND window) {
        return (SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) == 0;
    }
    static UINT textFlags(HWND window) {
        return (SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;
    }
    Entry* find(HWND window) const {
        for (const auto& entry : entries_) if (entry->window == window) return entry.get();
        return nullptr;
    }
    void add(HWND window, Kind kind) {
        if (!window || find(window)) return;
        // Never free item data while a re-entrant native control callback still
        // has a reference to it. Ordinary dialog creation is a safe prune point.
        if (callbackDepth_ == 0)
            entries_.erase(std::remove_if(entries_.begin(),entries_.end(),
                [](const auto& entry) { return !entry->window; }),entries_.end());
        auto entry = std::make_unique<Entry>();
        entry->owner = this; entry->window = window; entry->kind = kind;
        if (!SetWindowSubclass(window, subclassProc, subclassId, reinterpret_cast<DWORD_PTR>(entry.get()))) return;
        entries_.push_back(std::move(entry));
    }
    void drawButton(HWND window, HDC dc, RECT rectangle, bool hot) const {
        const int saved = SaveDC(dc); font(window, dc); SetBkMode(dc, TRANSPARENT);
        const auto style = GetWindowLongPtrW(window, GWL_STYLE) & BS_TYPEMASK;
        const auto state = SendMessageW(window, BM_GETSTATE, 0, 0);
        const bool enabled = IsWindowEnabled(window) != FALSE;
        const bool pressed = (state & BST_PUSHED) != 0;
        const bool focus = (state & BST_FOCUS) != 0 || GetFocus() == window;
        const bool checkbox = style == BS_CHECKBOX || style == BS_AUTOCHECKBOX ||
            style == BS_3STATE || style == BS_AUTO3STATE;
        const bool radio = style == BS_RADIOBUTTON || style == BS_AUTORADIOBUTTON;
        const auto textColor = enabled ? palette_.text : palette_.muted;
        fill(dc, rectangle, checkbox || radio ? palette_.background :
            pressed ? palette_.selection : hot && enabled ? palette_.hover : palette_.control);
        RECT caption = rectangle;
        if (checkbox || radio) {
            const int size = scaled(window, 16), inset = scaled(window, 2);
            RECT mark{rectangle.left + inset, rectangle.top + (rectangle.bottom-rectangle.top-size)/2,
                      rectangle.left+inset+size, rectangle.top+(rectangle.bottom-rectangle.top-size)/2+size};
            const auto checked = SendMessageW(window, BM_GETCHECK, 0, 0);
            fill(dc, mark, pressed || (hot && enabled) ? palette_.hover : palette_.control);
            outline(dc, mark, focus || hot ? palette_.output : palette_.border);
            if (checked != BST_UNCHECKED) {
                HPEN pen = CreatePen(PS_SOLID, std::max(1, scaled(window, 2)), enabled ? palette_.contact : palette_.muted);
                HGDIOBJ oldPen = SelectObject(dc, pen);
                if (radio) {
                    HBRUSH brush = CreateSolidBrush(enabled ? palette_.contact : palette_.muted);
                    HGDIOBJ oldBrush = SelectObject(dc, brush);
                    Ellipse(dc,mark.left+size/4,mark.top+size/4,mark.right-size/4,mark.bottom-size/4);
                    SelectObject(dc, oldBrush); DeleteObject(brush);
                } else if (checked == BST_INDETERMINATE) {
                    MoveToEx(dc,mark.left+size/4,mark.top+size/2,nullptr); LineTo(dc,mark.right-size/4,mark.top+size/2);
                } else {
                    MoveToEx(dc,mark.left+size/5,mark.top+size/2,nullptr);
                    LineTo(dc,mark.left+size*2/5,mark.top+size*3/4);
                    LineTo(dc,mark.left+size*4/5,mark.top+size/4);
                }
                SelectObject(dc,oldPen); DeleteObject(pen);
            }
            caption.left = mark.right + scaled(window, 7);
        } else {
            outline(dc, rectangle, focus ? palette_.output : palette_.border);
            InflateRect(&caption,-scaled(window,5),-scaled(window,2));
        }
        SetTextColor(dc, pressed && !checkbox && !radio ? palette_.selectionText : textColor);
        const auto value = label(window);
        DrawTextW(dc, value.c_str(), int(value.size()), &caption,
            (checkbox || radio ? DT_LEFT : DT_CENTER) | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | textFlags(window));
        if (focus && showFocus(window)) {
            RECT focusRectangle = checkbox || radio ? caption : rectangle;
            InflateRect(&focusRectangle,-scaled(window,3),-scaled(window,3));
            DrawFocusRect(dc,&focusRectangle);
        }
        RestoreDC(dc,saved);
    }
    void drawCombo(Entry& entry, HDC dc) const {
        RECT rectangle{}; GetClientRect(entry.window,&rectangle);
        const int saved = SaveDC(dc); font(entry.window,dc); SetBkMode(dc,TRANSPARENT);
        const bool enabled = IsWindowEnabled(entry.window) != FALSE;
        const bool open = SendMessageW(entry.window,CB_GETDROPPEDSTATE,0,0) != 0;
        fill(dc,rectangle,palette_.control);
        outline(dc,rectangle,GetFocus() == entry.window ? palette_.output : palette_.border);
        const int arrowWidth = std::max(scaled(entry.window,22), GetSystemMetricsForDpi(SM_CXVSCROLL,controlDpi(entry.window)));
        RECT arrow{std::max(rectangle.left,rectangle.right-arrowWidth),rectangle.top+1,rectangle.right-1,rectangle.bottom-1};
        fill(dc,arrow,open ? palette_.selection : entry.hot && enabled ? palette_.hover : palette_.control);
        triangle(dc,(arrow.left+arrow.right)/2,(arrow.top+arrow.bottom)/2,scaled(entry.window,4),false,
                 !enabled ? palette_.muted : open ? palette_.selectionText : palette_.text);
        RECT caption{rectangle.left+scaled(entry.window,7),rectangle.top+1,arrow.left-scaled(entry.window,3),rectangle.bottom-1};
        const int selection = int(SendMessageW(entry.window,CB_GETCURSEL,0,0));
        const auto value = comboLabel(entry.window,selection);
        SetTextColor(dc,enabled ? palette_.text : palette_.muted);
        DrawTextW(dc,value.c_str(),int(value.size()),&caption,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        if (GetFocus() == entry.window && showFocus(entry.window)) {
            InflateRect(&caption,-1,-scaled(entry.window,2)); DrawFocusRect(dc,&caption);
        }
        RestoreDC(dc,saved);
    }
    void drawSpin(Entry& entry, HDC dc) const {
        RECT rectangle{}; GetClientRect(entry.window,&rectangle);
        const bool enabled = IsWindowEnabled(entry.window) != FALSE;
        fill(dc,rectangle,palette_.control);
        const int middle = (rectangle.top+rectangle.bottom)/2;
        for (int part = 1; part <= 2; ++part) {
            RECT button{rectangle.left,part == 1 ? rectangle.top : middle,rectangle.right,part == 1 ? middle : rectangle.bottom};
            const bool pressed = GetCapture() == entry.window && entry.pressedPart == part;
            fill(dc,button,pressed ? palette_.selection : enabled && entry.hotPart == part ? palette_.hover : palette_.control);
            outline(dc,button,palette_.border);
            triangle(dc,(button.left+button.right)/2,(button.top+button.bottom)/2,scaled(entry.window,3),part == 1,
                     !enabled ? palette_.muted : pressed ? palette_.selectionText : palette_.text);
        }
    }
    void drawFrame(HWND window, HDC dc) const {
        RECT rectangle{}; GetWindowRect(window,&rectangle);
        rectangle.right -= rectangle.left; rectangle.bottom -= rectangle.top; rectangle.left = rectangle.top = 0;
        outline(dc,rectangle,GetFocus() == window ? palette_.output : palette_.border);
    }
    void drawListFrame(HWND window, HDC dc) const {
        RECT windowRectangle{}; GetWindowRect(window,&windowRectangle);
        RECT rectangle{0,0,windowRectangle.right-windowRectangle.left,windowRectangle.bottom-windowRectangle.top};
        outline(dc,rectangle,palette_.border);
        SCROLLBARINFO scroll{}; scroll.cbSize = sizeof(scroll);
        if (!GetScrollBarInfo(window,OBJID_VSCROLL,&scroll) ||
            (scroll.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)) != 0) return;
        RECT bar = scroll.rcScrollBar; OffsetRect(&bar,-windowRectangle.left,-windowRectangle.top);
        fill(dc,bar,palette_.control);
        const int buttonHeight = scroll.dxyLineButton;
        RECT top{bar.left,bar.top,bar.right,bar.top+buttonHeight};
        RECT bottom{bar.left,bar.bottom-buttonHeight,bar.right,bar.bottom};
        fill(dc,top,(scroll.rgstate[1] & STATE_SYSTEM_PRESSED) ? palette_.selection : palette_.control);
        fill(dc,bottom,(scroll.rgstate[5] & STATE_SYSTEM_PRESSED) ? palette_.selection : palette_.control);
        triangle(dc,(top.left+top.right)/2,(top.top+top.bottom)/2,scaled(window,3),true,palette_.text);
        triangle(dc,(bottom.left+bottom.right)/2,(bottom.top+bottom.bottom)/2,scaled(window,3),false,palette_.text);
        if (scroll.xyThumbBottom > scroll.xyThumbTop) {
            RECT thumb{bar.left+scaled(window,2),bar.top+scroll.xyThumbTop,
                       bar.right-scaled(window,2),bar.top+scroll.xyThumbBottom};
            fill(dc,thumb,(scroll.rgstate[3] & STATE_SYSTEM_PRESSED) ? palette_.selection : palette_.border);
        }
    }
    void repaintNonClient(Entry& entry) const {
        if (!entry.window || !IsWindowVisible(entry.window)) return;
        HDC dc = GetWindowDC(entry.window);
        if (!dc) return;
        if (entry.kind == Kind::ComboList) drawListFrame(entry.window,dc);
        else drawFrame(entry.window,dc);
        ReleaseDC(entry.window,dc);
    }
    void syncList(HWND combo) {
        COMBOBOXINFO info{}; info.cbSize = sizeof(info);
        if (!GetComboBoxInfo(combo,&info) || !info.hwndList) return;
        add(info.hwndList,Kind::ComboList);
        SetWindowTheme(info.hwndList,L"",L"");
        // Native scrollbar tracking uses a modal loop. Repaint its visible
        // non-client area during that loop without replacing its interactions.
        if (IsWindowVisible(info.hwndList)) {
            SetTimer(info.hwndList,scrollbarTimer,16,nullptr);
            if (auto* entry = find(info.hwndList)) repaintNonClient(*entry);
        }
    }
    static LRESULT CALLBACK subclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR, DWORD_PTR reference) {
        auto& entry = *reinterpret_cast<Entry*>(reference);
        auto& owner = *entry.owner;
        struct CallbackScope {
            unsigned& depth;
            explicit CallbackScope(unsigned& value) : depth(value) { ++depth; }
            ~CallbackScope() { --depth; }
        } scope(owner.callbackDepth_);
        if (message == WM_NCDESTROY) {
            KillTimer(window,scrollbarTimer); RemoveWindowSubclass(window,subclassProc,subclassId);
            entry.window = nullptr; return DefSubclassProc(window,message,wParam,lParam);
        }
        if (message == WM_MOUSEMOVE) {
            TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,window,0}; TrackMouseEvent(&track);
            RECT rectangle{}; GetClientRect(window,&rectangle);
            const int part = GET_Y_LPARAM(lParam) < (rectangle.bottom-rectangle.top)/2 ? 1 : 2;
            if (!entry.hot || entry.hotPart != part) {
                entry.hot = true; entry.hotPart = part; InvalidateRect(window,nullptr,FALSE);
            }
        } else if (message == WM_MOUSELEAVE) {
            entry.hot = false; entry.hotPart = 0; InvalidateRect(window,nullptr,FALSE);
        } else if (message == WM_LBUTTONDOWN && entry.kind == Kind::Spin) {
            RECT rectangle{}; GetClientRect(window,&rectangle);
            entry.pressedPart = GET_Y_LPARAM(lParam) < rectangle.bottom/2 ? 1 : 2;
        } else if (message == WM_LBUTTONUP || message == WM_CAPTURECHANGED || message == WM_CANCELMODE) {
            entry.pressedPart = 0; InvalidateRect(window,nullptr,FALSE);
        }
        if (message == WM_TIMER && wParam == scrollbarTimer && entry.kind == Kind::ComboList) {
            if (IsWindowVisible(window)) owner.repaintNonClient(entry);
            else KillTimer(window,scrollbarTimer);
            return 0;
        }
        if ((message == WM_PAINT || message == WM_PRINT || message == WM_PRINTCLIENT) &&
            (entry.kind == Kind::Combo || entry.kind == Kind::Spin || entry.kind == Kind::Button)) {
            PAINTSTRUCT paint{};
            HDC dc = message == WM_PAINT ? BeginPaint(window,&paint) : reinterpret_cast<HDC>(wParam);
            if (entry.kind == Kind::Combo) owner.drawCombo(entry,dc);
            else if (entry.kind == Kind::Spin) owner.drawSpin(entry,dc);
            else { RECT rectangle{}; GetClientRect(window,&rectangle); owner.drawButton(window,dc,rectangle,entry.hot); }
            if (message == WM_PAINT) EndPaint(window,&paint);
            return 0;
        }
        if (message == WM_ERASEBKGND && (entry.kind == Kind::Combo || entry.kind == Kind::Spin ||
                                      entry.kind == Kind::ComboList || entry.kind == Kind::Button)) {
            RECT rectangle{}; GetClientRect(window,&rectangle);
            fill(reinterpret_cast<HDC>(wParam),rectangle,owner.palette_.control); return 1;
        }
        const LRESULT result = DefSubclassProc(window,message,wParam,lParam);
        if (!entry.window) return result;
        if (entry.kind == Kind::Combo && message == WM_SETFONT) {
            const int height = scaled(window,24);
            SendMessageW(window,CB_SETITEMHEIGHT,0,height);
            SendMessageW(window,CB_SETITEMHEIGHT,WPARAM(-1),height);
        }
        if ((entry.kind == Kind::Edit || entry.kind == Kind::ComboList) &&
            (message == WM_NCPAINT || message == WM_PAINT || message == WM_SETFOCUS || message == WM_KILLFOCUS ||
             message == WM_NCMOUSEMOVE || message == WM_NCLBUTTONUP || message == WM_VSCROLL)) owner.repaintNonClient(entry);
        if ((message == WM_PRINT || message == WM_PRINTCLIENT) && (entry.kind == Kind::Edit || entry.kind == Kind::ComboList)) {
            if (entry.kind == Kind::ComboList) owner.drawListFrame(window,reinterpret_cast<HDC>(wParam));
            else owner.drawFrame(window,reinterpret_cast<HDC>(wParam));
        }
        if (entry.kind == Kind::Combo && (message == WM_LBUTTONDOWN || message == WM_KEYDOWN || message == CB_SHOWDROPDOWN)) owner.syncList(window);
        if (entry.kind == Kind::Combo || entry.kind == Kind::Spin || entry.kind == Kind::Button) {
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_ENABLE ||
                message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_KEYDOWN ||
                message == WM_KEYUP || message == WM_UPDATEUISTATE || message == BM_SETCHECK || message == BM_SETSTATE ||
                message == CB_SETCURSEL || message == CB_SHOWDROPDOWN || message == UDM_SETPOS32)
                InvalidateRect(window,nullptr,FALSE);
        }
        return result;
    }
public:
    UiTheme() { setPalette(palette_); }
    UiTheme(const UiTheme&) = delete;
    UiTheme& operator=(const UiTheme&) = delete;
    ~UiTheme() {
        for (const auto& entry : entries_) if (entry->window && IsWindow(entry->window)) {
            KillTimer(entry->window,scrollbarTimer); RemoveWindowSubclass(entry->window,subclassProc,subclassId);
        }
        for (auto brush : {backgroundBrush_,controlBrush_,surfaceBrush_}) if (brush) DeleteObject(brush);
    }
    void setPalette(const ThemePalette& palette, bool highContrast = false) {
        palette_ = palette; highContrast_ = highContrast;
        for (auto brush : {backgroundBrush_,controlBrush_,surfaceBrush_}) if (brush) DeleteObject(brush);
        backgroundBrush_ = CreateSolidBrush(palette_.background);
        controlBrush_ = CreateSolidBrush(palette_.control);
        surfaceBrush_ = CreateSolidBrush(palette_.surface);
        refresh();
    }
    const ThemePalette& palette() const { return palette_; }
    void attach(HWND window) {
        if (!window || find(window)) return;
        wchar_t name[64]{}; GetClassNameW(window,name,64);
        if (_wcsicmp(name,L"Button") == 0) add(window,Kind::Button);
        else if (_wcsicmp(name,L"ComboBox") == 0) {
            add(window,Kind::Combo); SetWindowTheme(window,L"",L"");
            const int itemHeight = scaled(window,24);
            SendMessageW(window,CB_SETITEMHEIGHT,0,itemHeight);
            SendMessageW(window,CB_SETITEMHEIGHT,WPARAM(-1),itemHeight);
            syncList(window);
        } else if (_wcsicmp(name,L"Edit") == 0) { add(window,Kind::Edit); SetWindowTheme(window,L"",L""); }
        else if (_wcsicmp(name,UPDOWN_CLASSW) == 0) { add(window,Kind::Spin); SetWindowTheme(window,L"",L""); }
    }
    void refresh() {
        for (const auto& entry : entries_) if (entry->window && IsWindow(entry->window)) {
            if (entry->kind == Kind::Combo) {
                const int height = scaled(entry->window,24);
                SendMessageW(entry->window,CB_SETITEMHEIGHT,0,height);
                SendMessageW(entry->window,CB_SETITEMHEIGHT,WPARAM(-1),height);
            }
            RedrawWindow(entry->window,nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
        }
    }
    bool handleNotify(LPARAM parameter, LRESULT& result) const {
        if (!parameter) return false;
        const auto& header = *reinterpret_cast<NMHDR*>(parameter);
        if (header.code != NM_CUSTOMDRAW) return false;
        const auto* entry = find(header.hwndFrom);
        if (!entry || entry->kind != Kind::Button) return false;
        const auto& draw = *reinterpret_cast<NMCUSTOMDRAW*>(parameter);
        if (highContrast_) { result = CDRF_DODEFAULT; return true; }
        if (draw.dwDrawStage == CDDS_PREPAINT || draw.dwDrawStage == CDDS_PREERASE) {
            drawButton(entry->window,draw.hdc,draw.rc,entry->hot);
            result = CDRF_SKIPDEFAULT; return true;
        }
        result = CDRF_DODEFAULT; return true;
    }
    bool handleDrawItem(const DRAWITEMSTRUCT& draw) const {
        if (draw.CtlType != ODT_COMBOBOX) return false;
        const int saved = SaveDC(draw.hDC); font(draw.hwndItem,draw.hDC); SetBkMode(draw.hDC,TRANSPARENT);
        const bool selected = (draw.itemState & ODS_SELECTED) != 0;
        fill(draw.hDC,draw.rcItem,selected ? palette_.selection : palette_.control);
        const auto value = comboLabel(draw.hwndItem,draw.itemID == UINT(-1) ? -1 : int(draw.itemID));
        RECT caption = draw.rcItem; caption.left += scaled(draw.hwndItem,7); caption.right -= scaled(draw.hwndItem,4);
        SetTextColor(draw.hDC,(draw.itemState & ODS_DISABLED) ? palette_.muted : selected ? palette_.selectionText : palette_.text);
        DrawTextW(draw.hDC,value.c_str(),int(value.size()),&caption,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        if ((draw.itemState & ODS_FOCUS) && !(draw.itemState & ODS_NOFOCUSRECT)) DrawFocusRect(draw.hDC,&draw.rcItem);
        RestoreDC(draw.hDC,saved); return true;
    }
    bool handleMeasureItem(MEASUREITEMSTRUCT& measure) const {
        if (measure.CtlType != ODT_COMBOBOX) return false;
        HWND window = nullptr;
        for (const auto& entry : entries_) if (entry->window && entry->kind == Kind::Combo &&
            UINT(GetDlgCtrlID(entry->window)) == measure.CtlID) { window = entry->window; break; }
        measure.itemHeight = UINT(window ? scaled(window,24) : MulDiv(24,int(GetDpiForSystem()),96));
        return true;
    }
    HBRUSH colorControl(HDC dc, HWND window, UINT message) const {
        SetTextColor(dc,IsWindowEnabled(window) ? palette_.text : palette_.muted);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX ||
            (message == WM_CTLCOLORSTATIC && find(window) && find(window)->kind == Kind::Edit)) {
            SetBkColor(dc,palette_.control); return controlBrush_;
        }
        SetBkColor(dc,palette_.background); SetBkMode(dc,TRANSPARENT); return backgroundBrush_;
    }
};
} // namespace morse
