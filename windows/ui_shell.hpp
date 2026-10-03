// SPDX-License-Identifier: GPL-3.0-only
// Included inside main.cpp's private namespace: UI shell never owns COM/audio.
#pragma once

constexpr wchar_t SHELL_CLASS_NAME[] = L"MorseBridgeDialog";
enum class DialogKind { Settings, About, Update };
struct ShellState {
    WindowIcons icons;
    DialogKind kind = DialogKind::About;
    morse::ThemeId previous = morse::ThemeId::System;
    UINT dpi = 96;
    int defaultId = IDCANCEL;
    HWND choice = nullptr, status = nullptr, release = nullptr, close = nullptr;
    HFONT font = nullptr, title = nullptr;
    struct Control { HWND window; int x,y,w,h; bool heading; };
    std::vector<Control> controls;
} shell;

struct MenuEntry {
    MSAAMENUINFO accessibility{}; // Must be first: preserves native MSAA names.
    const wchar_t* text;
    UINT id;
    bool heading;
};
std::array<MenuEntry,7> menuEntries{{
    {{}, L"&Datei", 0, true}, {{}, L"&Optionen", 0, true}, {{}, L"&Hilfe", 0, true},
    {{}, L"&Beenden", ID_CLOSE, false}, {{}, L"&Einstellungen…", ID_SETTINGS, false},
    {{}, L"Nach &Updates suchen…", ID_UPDATE, false}, {{}, L"&Über MorseBridge…", ID_ABOUT, false}
}};

HMENU makeMenu() {
    HMENU bar = CreateMenu();
    std::array<HMENU,3> popups{{CreatePopupMenu(),CreatePopupMenu(),CreatePopupMenu()}};
    for (size_t i=0; i<menuEntries.size(); ++i) {
        auto& entry = menuEntries[i];
        entry.accessibility.dwMSAASignature = MSAA_MENU_SIG;
        entry.accessibility.cchWText = DWORD(wcslen(entry.text));
        entry.accessibility.pszWText = const_cast<wchar_t*>(entry.text);
        MENUITEMINFOW item{}; item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STRING;
        item.fType = MFT_OWNERDRAW; item.dwItemData = reinterpret_cast<ULONG_PTR>(&entry);
        item.dwTypeData = const_cast<wchar_t*>(entry.text);
        if (entry.heading) { item.fMask |= MIIM_SUBMENU; item.hSubMenu = popups[i]; }
        else { item.fMask |= MIIM_ID; item.wID = entry.id; }
        const size_t popup = i < 3 ? i : i == 3 ? 0 : i == 4 ? 1 : 2;
        InsertMenuItemW(i < 3 ? bar : popups[popup], UINT(-1), TRUE, &item);
    }
    return bar;
}

void paintMenuEntry(HDC dc, RECT rect, const MenuEntry& entry, bool selected, bool disabled, bool hidePrefix) {
    const auto& p = app.palette;
    fillRect(dc, rect, selected ? p.selection : p.background);
    rect.left += app.scale(entry.heading ? 10 : 14);
    rect.right -= app.scale(12);
    HGDIOBJ font = SelectObject(dc, app.font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, disabled ? p.muted : selected ? p.selectionText : p.text);
    DrawTextW(dc, entry.text, -1, &rect, DT_SINGLELINE | DT_VCENTER | (hidePrefix ? DT_HIDEPREFIX : 0));
    SelectObject(dc, font);
}

bool drawMenuItem(const DRAWITEMSTRUCT& item) {
    if (item.CtlType != ODT_MENU || !item.itemData) return false;
    const auto* entry = reinterpret_cast<const MenuEntry*>(item.itemData);
    paintMenuEntry(item.hDC,item.rcItem,*entry,(item.itemState & ODS_SELECTED) != 0,
                   (item.itemState & (ODS_DISABLED | ODS_GRAYED)) != 0, (item.itemState & ODS_NOACCEL) != 0);
    return true;
}

bool measureMenuItem(MEASUREITEMSTRUCT& item) {
    if (item.CtlType != ODT_MENU || !item.itemData) return false;
    const auto* entry = reinterpret_cast<const MenuEntry*>(item.itemData);
    HDC dc = GetDC(app.window); HGDIOBJ old = SelectObject(dc, app.font);
    RECT r{}; DrawTextW(dc,entry->text,-1,&r,DT_SINGLELINE | DT_CALCRECT);
    SelectObject(dc,old); ReleaseDC(app.window,dc);
    item.itemWidth = UINT(r.right + app.scale(entry->heading ? 26 : 38));
    item.itemHeight = UINT(app.scale(26));
    return true;
}

LRESULT menuMnemonic(WPARAM key, HMENU menu) {
    const int count = GetMenuItemCount(menu);
    for (int i=0; i<count; ++i) {
        MENUITEMINFOW info{}; info.cbSize=sizeof(info); info.fMask=MIIM_DATA | MIIM_STATE | MIIM_SUBMENU;
        if (!GetMenuItemInfoW(menu,UINT(i),TRUE,&info) || !info.dwItemData || (info.fState & MFS_DISABLED)) continue;
        const auto* entry = reinterpret_cast<const MenuEntry*>(info.dwItemData);
        const auto* mnemonic = wcschr(entry->text,L'&');
        if (mnemonic && towupper(mnemonic[1]) == towupper(wchar_t(LOWORD(key))))
            return MAKELRESULT(i,MNC_EXECUTE);
    }
    return MAKELRESULT(0,MNC_IGNORE);
}

// Fill the entire native menu bar, including the otherwise system-colored gap.
// The system still owns layout, hit testing, keyboard navigation and popups.
void paintMenuBar(HWND hwnd) {
    MENUBARINFO info{}; info.cbSize=sizeof(info);
    if (!GetMenuBarInfo(hwnd,OBJID_MENU,0,&info) || !info.hMenu) return;
    RECT window{}; GetWindowRect(hwnd,&window);
    RECT bar = info.rcBar; OffsetRect(&bar,-window.left,-window.top);
    HDC dc = GetWindowDC(hwnd);
    fillRect(dc,bar,app.palette.background);
    for (int i=0; i<GetMenuItemCount(info.hMenu); ++i) {
        MENUITEMINFOW item{}; item.cbSize=sizeof(item); item.fMask=MIIM_DATA | MIIM_STATE;
        RECT rect{};
        if (!GetMenuItemInfoW(info.hMenu,UINT(i),TRUE,&item) || !item.dwItemData ||
            !GetMenuItemRect(hwnd,info.hMenu,UINT(i),&rect)) continue;
        OffsetRect(&rect,-window.left,-window.top);
        paintMenuEntry(dc,rect,*reinterpret_cast<const MenuEntry*>(item.dwItemData),
                       (item.fState & MFS_HILITE) != 0, (item.fState & MFS_DISABLED) != 0, false);
    }
    ReleaseDC(hwnd,dc);
}

void persistSettings() {
    if (app.renderTest) return;
    app.persistenceError = !morse::saveSettings(app.settingsFile,app.settings);
    if (app.window) InvalidateRect(app.window,nullptr,FALSE);
}

void themeWindowFrame(HWND hwnd) {
    const BOOL dark = app.palette.dark && !morse::highContrastMode();
    // Documented on Windows 11; Windows 10 may retain its system frame.
    DwmSetWindowAttribute(hwnd,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));
}

void applyAppearance(morse::ThemeId id) {
    app.displayTheme = id;
    const bool contrast = morse::highContrastMode();
    app.palette = contrast ? morse::systemPalette() : morse::paletteFor(id,morse::systemDarkMode(),false);
    app.theme.setPalette(app.palette,contrast); app.theme.refresh();
    HBRUSH brush = CreateSolidBrush(app.palette.background);
    if (app.window) {
        MENUINFO info{}; info.cbSize=sizeof(info); info.fMask=MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
        info.hbrBack=brush; SetMenuInfo(GetMenu(app.window),&info);
        themeWindowFrame(app.window); DrawMenuBar(app.window);
        RedrawWindow(app.window,nullptr,nullptr,RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    }
    if (app.dialog) {
        themeWindowFrame(app.dialog);
        RedrawWindow(app.dialog,nullptr,nullptr,RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    }
    if (app.menuBrush) DeleteObject(app.menuBrush);
    app.menuBrush=brush;
}

void layoutShell() {
    for (const auto& control : shell.controls) {
        MoveWindow(control.window,MulDiv(control.x,int(shell.dpi),96),MulDiv(control.y,int(shell.dpi),96),
                   MulDiv(control.w,int(shell.dpi),96),MulDiv(control.h,int(shell.dpi),96),TRUE);
        SendMessageW(control.window,WM_SETFONT,reinterpret_cast<WPARAM>(control.heading ? shell.title : shell.font),TRUE);
    }
}

void shellFonts() {
    if (shell.font) DeleteObject(shell.font);
    if (shell.title) DeleteObject(shell.title);
    auto font = [](int pixels,int weight) {
        return CreateFontW(-MulDiv(pixels,int(shell.dpi),96),0,0,0,weight,FALSE,FALSE,FALSE,
                          DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    };
    shell.font=font(14,FW_NORMAL); shell.title=font(22,FW_SEMIBOLD); layoutShell();
}

void closeShell(bool accept) {
    const HWND window=app.dialog;
    if (!window) return;
    const bool wasForeground = GetForegroundWindow() == window;
    if (shell.kind == DialogKind::Settings) {
        if (accept) { app.settings.theme=app.displayTheme; persistSettings(); }
        else applyAppearance(shell.previous);
    } else if (shell.kind == DialogKind::Update) {
        app.updates.shutdown();
        if (app.window) { EnableMenuItem(GetMenu(app.window),ID_UPDATE,MF_BYCOMMAND | MF_ENABLED); DrawMenuBar(app.window); }
    }
    DestroyWindow(window);
    if (IsWindow(app.window)) {
        EnableWindow(app.window,TRUE);
        if (wasForeground && !app.stop) SetForegroundWindow(app.window);
    }
}

void openShell(DialogKind kind);
void refreshUpdateDialog() {
    const auto result=app.updates.result();
    if (app.dialog && shell.kind == DialogKind::Update) {
        SetWindowTextW(shell.status,result.message.c_str());
        EnableWindow(shell.release,result.state == morse::UpdateState::Available);
        SetWindowTextW(shell.close,result.state == morse::UpdateState::Checking ? L"&Abbrechen" : L"&Schließen");
    }
    if (app.window) {
        EnableMenuItem(GetMenu(app.window),ID_UPDATE,MF_BYCOMMAND | (app.updates.running() ? MF_GRAYED : MF_ENABLED));
        DrawMenuBar(app.window);
    }
}

bool openLink(HWND owner, const wchar_t* url) {
    const auto status=reinterpret_cast<INT_PTR>(ShellExecuteW(owner,L"open",url,nullptr,nullptr,SW_SHOWNORMAL));
    if (status>32) return true;
    MessageBoxW(owner,L"Der Browser konnte nicht geöffnet werden. Bitte die GitHub-Adresse manuell aufrufen.",L"MorseBridge",MB_OK | MB_ICONINFORMATION);
    return false;
}

LRESULT CALLBACK shellProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam) {
    switch (message) {
    case DM_GETDEFID: return MAKELRESULT(shell.defaultId,DC_HASDEFID);
    case DM_SETDEFID: {
        shell.defaultId=int(wParam);
        for (const auto& item : shell.controls) {
            wchar_t cls[32]{}; GetClassNameW(item.window,cls,32);
            if (wcscmp(cls,WC_BUTTONW)==0)
                SendMessageW(item.window,BM_SETSTYLE,GetDlgCtrlID(item.window)==shell.defaultId ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON,TRUE);
        }
        return TRUE;
    }
    case WM_CREATE: {
        app.dialog=hwnd; shell.dpi=GetDpiForWindow(hwnd);
        shell.icons.apply(hwnd,shell.dpi);
        auto control=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h,bool heading=false) {
            HWND child=CreateWindowExW(0,cls,text,WS_CHILD | WS_VISIBLE | style,x,y,w,h,hwnd,
                                      reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
            shell.controls.push_back({child,x,y,w,h,heading}); app.theme.attach(child); return child;
        };
        if (shell.kind == DialogKind::Settings) {
            control(WC_STATICW,L"Einstellungen",SS_LEFT,0,24,18,500,32,true);
            control(WC_STATICW,L"&Farbschema",SS_LEFT,0,24,65,160,24);
            shell.choice=control(WC_COMBOBOXW,L"",WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
                                 ID_THEME,190,61,340,280);
            for (const wchar_t* name : {L"System",L"Hell",L"Dunkel",L"Mitternacht",L"Amber",L"Matrix"})
                SendMessageW(shell.choice,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
            SendMessageW(shell.choice,CB_SETCURSEL,WPARAM(app.displayTheme),0);
            control(WC_STATICW,L"Die Tonhöhe im Hauptfenster wird ebenfalls gespeichert.\nCOM-Suche startet automatisch; Ton und Vordergrund starten aus.",SS_LEFT,0,24,108,506,48);
            shell.status=control(WC_STATICW,L"",SS_LEFT,0,24,158,506,38);
            control(WC_BUTTONW,L"&OK",WS_TABSTOP | BS_DEFPUSHBUTTON,IDOK,300,207,106,32);
            shell.close=control(WC_BUTTONW,L"&Abbrechen",WS_TABSTOP | BS_PUSHBUTTON,IDCANCEL,422,207,108,32);
        } else if (shell.kind == DialogKind::About) {
            control(WC_STATICW,L"MorseBridge",SS_LEFT,0,24,18,500,32,true);
            const std::wstring info=L"Version " MB_VERSION_WSTRING L"\nBuild (UTC): " MB_BUILD_UTC_WSTRING;
            control(WC_STATICW,info.c_str(),SS_LEFT,0,24,68,506,48);
            control(WC_STATICW,L"Junker M.T. → Arduino Nano → Leertaste\nEin Hobbyprojekt von Auragant · GPLv3\nMit Unterstützung von LLM/KI erstellt.",SS_LEFT,0,24,128,506,70);
            control(WC_LINK,L"<a>github.com/Auragant/Morsetaste</a>",WS_TABSTOP | LWS_TRANSPARENT,ID_GITHUB,24,213,506,26);
            control(WC_BUTTONW,L"&GitHub öffnen",WS_TABSTOP | BS_PUSHBUTTON,ID_GITHUB,24,258,148,34);
            control(WC_BUTTONW,L"Nach &Updates suchen",WS_TABSTOP | BS_PUSHBUTTON,ID_UPDATE,184,258,194,34);
            shell.close=control(WC_BUTTONW,L"&Schließen",WS_TABSTOP | BS_DEFPUSHBUTTON,IDCANCEL,422,258,108,34);
        } else {
            control(WC_STATICW,L"Nach Updates suchen",SS_LEFT,0,24,18,500,32,true);
            shell.status=control(WC_STATICW,L"GitHub wird geprüft …",SS_LEFT,ID_UPDATE_STATUS,24,70,506,86);
            control(WC_STATICW,L"Der Download erfolgt auf GitHub im Browser.",SS_LEFT,0,24,165,506,25);
            shell.release=control(WC_BUTTONW,L"&Release öffnen",WS_TABSTOP | BS_PUSHBUTTON,ID_RELEASE,24,213,158,34);
            EnableWindow(shell.release,FALSE);
            shell.close=control(WC_BUTTONW,L"&Abbrechen",WS_TABSTOP | BS_DEFPUSHBUTTON,IDCANCEL,422,213,108,34);
        }
        shellFonts(); themeWindowFrame(hwnd); return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_THEME:
            if (HIWORD(wParam)==CBN_SELCHANGE) {
                const LRESULT selected=SendMessageW(shell.choice,CB_GETCURSEL,0,0);
                if (selected>=0 && selected<=5) applyAppearance(static_cast<morse::ThemeId>(selected));
            } return 0;
        case IDOK: closeShell(true); return 0;
        case IDCANCEL: closeShell(false); return 0;
        case ID_GITHUB: openLink(hwnd,L"https://github.com/Auragant/Morsetaste"); return 0;
        case ID_RELEASE: openLink(hwnd,L"https://github.com/Auragant/Morsetaste/releases/latest"); return 0;
        case ID_UPDATE: closeShell(false); openShell(DialogKind::Update); return 0;
        } break;
    case WM_NOTIFY: {
        const auto* header=reinterpret_cast<const NMHDR*>(lParam);
        if (header->idFrom==ID_GITHUB) {
            if (header->code==NM_CLICK || header->code==NM_RETURN) {
                openLink(hwnd,L"https://github.com/Auragant/Morsetaste"); return 0;
            }
            if (header->code==NM_CUSTOMDRAW) {
                const auto* draw=reinterpret_cast<const NMCUSTOMDRAW*>(lParam);
                if (draw->dwDrawStage==CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
                SetTextColor(draw->hdc,app.palette.output); SetBkColor(draw->hdc,app.palette.background);
                return CDRF_DODEFAULT;
            }
        }
        LRESULT result=0; if (app.theme.handleNotify(lParam,result)) return result; break;
    }
    case WM_MEASUREITEM:
        if (app.theme.handleMeasureItem(*reinterpret_cast<MEASUREITEMSTRUCT*>(lParam))) return TRUE;
        break;
    case WM_DRAWITEM:
        if (app.theme.handleDrawItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam))) return TRUE;
        break;
    case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT: case WM_CTLCOLORBTN: case WM_CTLCOLORLISTBOX:
        return reinterpret_cast<LRESULT>(app.theme.colorControl(reinterpret_cast<HDC>(wParam),reinterpret_cast<HWND>(lParam),message));
    case WM_ERASEBKGND: {
        RECT rect{}; GetClientRect(hwnd,&rect); fillRect(reinterpret_cast<HDC>(wParam),rect,app.palette.background); return 1;
    }
    case WM_DPICHANGED: {
        shell.dpi=HIWORD(wParam); shellFonts();
        shell.icons.apply(hwnd,shell.dpi);
        const auto* rect=reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hwnd,nullptr,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER | SWP_NOACTIVATE);
        app.theme.refresh();
        return 0;
    }
    case WM_CLOSE: closeShell(false); return 0;
    case WM_DESTROY:
        app.dialog=nullptr; shell.controls.clear();
        if (shell.font) { DeleteObject(shell.font); shell.font=nullptr; }
        if (shell.title) { DeleteObject(shell.title); shell.title=nullptr; }
        return 0;
    case WM_NCDESTROY: {
        const LRESULT result=DefWindowProcW(hwnd,message,wParam,lParam);
        shell.icons.clear(); return result;
    }
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}

void openShell(DialogKind kind) {
    if (app.dialog) { SetForegroundWindow(app.dialog); return; }
    shell.kind=kind; shell.previous=app.displayTheme; shell.controls.clear();
    shell.defaultId=kind==DialogKind::Settings ? IDOK : IDCANCEL;
    shell.choice=nullptr; shell.status=nullptr; shell.release=nullptr; shell.close=nullptr;
    const DWORD style=WS_POPUP | WS_CAPTION | WS_SYSMENU;
    // A dialog modal frame hides the caption icon; owner disabling below keeps
    // these windows modal while a standard frame displays the application icon.
    const DWORD ex=WS_EX_CONTROLPARENT;
    RECT size{0,0,app.scale(554),app.scale(kind==DialogKind::Settings ? 260 : kind==DialogKind::About ? 320 : 272)};
    AdjustWindowRectExForDpi(&size,style,FALSE,ex,app.dpi);
    RECT owner{}; GetWindowRect(app.window,&owner);
    const int width=size.right-size.left, height=size.bottom-size.top;
    const wchar_t* title=kind==DialogKind::Settings ? L"MorseBridge – Einstellungen" : kind==DialogKind::About ? L"Über MorseBridge" : L"MorseBridge – Updateprüfung";
    HWND window=CreateWindowExW(ex,SHELL_CLASS_NAME,title,style,owner.left+(owner.right-owner.left-width)/2,
                               owner.top+(owner.bottom-owner.top-height)/2,width,height,app.window,nullptr,GetModuleHandleW(nullptr),nullptr);
    if (!window) return;
    EnableWindow(app.window,FALSE);
    if (!app.renderTest) { ShowWindow(window,SW_SHOW); SetForegroundWindow(window); }
    SetFocus(shell.choice ? shell.choice : shell.close);
    if (kind==DialogKind::Update) {
        app.updates.start(app.window,WM_UPDATE_READY,{MB_VERSION_MAJOR,MB_VERSION_MINOR,MB_VERSION_PATCH});
        refreshUpdateDialog();
    }
}
