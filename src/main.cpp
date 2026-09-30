#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <commctrl.h>

#include "Config.h"
#include "Logger.h"
#include "Bot.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "msimg32.lib")

static const wchar_t* kClass = L"FarshadPMWnd";
static const int kW = 720;
static const int kH = 640;

static HWND gHwnd = nullptr;
static HWND gLogList = nullptr;
static BotEngine* gBot = nullptr;
static int gCaptureHotkey = 0; // 1=activate 2=armed 3=fast 4=stop

// Colors — neon cyber
static COLORREF C_BG = RGB(6, 8, 12);
static COLORREF C_PANEL = RGB(12, 16, 24);
static COLORREF C_BORDER = RGB(0, 160, 255);
static COLORREF C_ACCENT = RGB(0, 200, 255);
static COLORREF C_TEXT = RGB(230, 240, 255);
static COLORREF C_MUTED = RGB(120, 140, 160);
static COLORREF C_OK = RGB(40, 220, 140);
static COLORREF C_WARN = RGB(255, 180, 40);
static COLORREF C_ERR = RGB(255, 80, 90);

static HFONT gFontTitle = nullptr;
static HFONT gFontUI = nullptr;
static HFONT gFontSmall = nullptr;
static HBRUSH gBrushBg = nullptr;
static HBRUSH gBrushPanel = nullptr;

struct UiBtn {
    RECT rc{};
    std::wstring label;
    int id = 0;
    bool hover = false;
};

static std::vector<UiBtn> gBtns;

enum {
    ID_ARM = 1,
    ID_FAST,
    ID_SAVE,
    ID_CLEARLOG,
    ID_HK_ACTIVATE,
    ID_HK_ARMED,
    ID_HK_FAST,
    ID_HK_STOP,
    ID_TOL_MINUS,
    ID_TOL_PLUS,
    ID_RXN_MINUS,
    ID_RXN_PLUS,
};

static void RefreshLogList() {
    if (!gLogList) return;
    SendMessageW(gLogList, LB_RESETCONTENT, 0, 0);
    auto snap = Logger::Instance().Snapshot();
    for (auto& e : snap) {
        const wchar_t* tag = L"INFO";
        if (e.level == LogLevel::Warn) tag = L"WARN";
        else if (e.level == LogLevel::Error) tag = L"ERR ";
        else if (e.level == LogLevel::Success) tag = L"OK  ";
        else if (e.level == LogLevel::Debug) tag = L"DBG ";
        std::wstring line = L"[" + e.time + L"] " + tag + L"  " + e.message;
        SendMessageW(gLogList, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }
    int count = (int)SendMessageW(gLogList, LB_GETCOUNT, 0, 0);
    if (count > 0) SendMessageW(gLogList, LB_SETTOPINDEX, count - 1, 0);
}

static void OnLogUi() {
    if (gHwnd) PostMessageW(gHwnd, WM_APP + 1, 0, 0);
}

static void LayoutButtons() {
    gBtns.clear();
    auto add = [&](int x, int y, int w, int h, const std::wstring& t, int id) {
        UiBtn b;
        b.rc = { x, y, x + w, y + h };
        b.label = t;
        b.id = id;
        gBtns.push_back(b);
    };
    // Status toggles
    add(24, 150, 150, 36, L"ARM / DISARM", ID_ARM);
    add(186, 150, 180, 36, L"FAST SNIPER", ID_FAST);
    add(378, 150, 120, 36, L"SAVE", ID_SAVE);
    add(510, 150, 170, 36, L"CLEAR LOG", ID_CLEARLOG);

    // Hotkey rows — click to rebind
    add(400, 210, 160, 28, L"Change", ID_HK_ACTIVATE);
    add(400, 246, 160, 28, L"Change", ID_HK_ARMED);
    add(400, 282, 160, 28, L"Change", ID_HK_FAST);
    add(400, 318, 160, 28, L"Change", ID_HK_STOP);

    add(200, 360, 36, 28, L"-", ID_TOL_MINUS);
    add(280, 360, 36, 28, L"+", ID_TOL_PLUS);
    add(200, 396, 36, 28, L"-", ID_RXN_MINUS);
    add(280, 396, 36, 28, L"+", ID_RXN_PLUS);
}

static void DrawRoundRect(HDC hdc, RECT rc, COLORREF fill, COLORREF border, int radius = 10) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 2, border);
    HGDIOBJ oldBr = SelectObject(hdc, br);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(br);
    DeleteObject(pen);
}

static void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HGDIOBJ oldBmp = SelectObject(mem, bmp);

    FillRect(mem, &client, gBrushBg);

    // Header bar
    RECT header{ 0, 0, client.right, 120 };
    HBRUSH hdrBr = CreateSolidBrush(RGB(8, 12, 20));
    FillRect(mem, &header, hdrBr);
    DeleteObject(hdrBr);

    // Neon line under header
    HPEN neon = CreatePen(PS_SOLID, 2, C_ACCENT);
    HGDIOBJ oldPen = SelectObject(mem, neon);
    MoveToEx(mem, 0, 120, nullptr);
    LineTo(mem, client.right, 120);
    SelectObject(mem, oldPen);
    DeleteObject(neon);

    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, C_ACCENT);
    SelectObject(mem, gFontTitle);
    TextOutW(mem, 24, 18, L"FARSHAD PM", 10);

    SetTextColor(mem, C_TEXT);
    SelectObject(mem, gFontUI);
    TextOutW(mem, 24, 58, L"TRIGGER  /  AIMING  TOOL", 24);

    SetTextColor(mem, C_MUTED);
    SelectObject(mem, gFontSmall);
    TextOutW(mem, 24, 88, L"\x0633\x0627\x062e\x062a\x0647 \x0634\x062f\x0647 \x062a\x0648\x0633\x0637 \x0641\x0631\x0634\x0627\x062f\x06cc \x067e\x06cc \x0627\x0645   |   Support: @farshad_pm_org", 60);

    // Status panel
    RECT panel{ 16, 136, client.right - 16, 430 };
    DrawRoundRect(mem, panel, C_PANEL, RGB(20, 40, 60), 12);

    auto& c = Config();
    SelectObject(mem, gFontUI);
    SetTextColor(mem, c.armed ? C_OK : C_MUTED);
    std::wstring st = c.armed ? L"STATUS: ARMED" : L"STATUS: STANDBY";
    TextOutW(mem, 24, 200, st.c_str(), (int)st.size());

    SetTextColor(mem, c.fastSniperEnabled ? C_ACCENT : C_MUTED);
    std::wstring fs = c.fastSniperEnabled ? L"FAST SNIPER: ON  (Q+Q in 0.5s)" : L"FAST SNIPER: OFF";
    TextOutW(mem, 24, 228, fs.c_str(), (int)fs.size());

    if (gBot) {
        wchar_t buf[64];
        swprintf_s(buf, L"Triggers: %llu", gBot->TriggerCount());
        SetTextColor(mem, C_TEXT);
        TextOutW(mem, 24, 256, buf, (int)wcslen(buf));
    }

    // Hotkey labels
    SetTextColor(mem, C_MUTED);
    SelectObject(mem, gFontSmall);
    TextOutW(mem, 24, 214, L"", 0); // spacer already used
    int y = 214;
    // redraw hotkey section cleanly at fixed y
    y = 214;
    (void)y;

    SetTextColor(mem, C_TEXT);
    SelectObject(mem, gFontUI);
    auto row = [&](int yy, const wchar_t* label, int vk) {
        SetTextColor(mem, C_MUTED);
        SelectObject(mem, gFontSmall);
        TextOutW(mem, 24, yy, label, (int)wcslen(label));
        SetTextColor(mem, C_ACCENT);
        SelectObject(mem, gFontUI);
        auto name = AppConfig::KeyName(vk);
        TextOutW(mem, 200, yy - 2, name.c_str(), (int)name.size());
    };
    row(214, L"Hold Activate", c.vkActivate);
    row(250, L"Toggle Armed", c.vkToggleArmed);
    row(286, L"Toggle Fast Sniper", c.vkToggleFastSniper);
    row(322, L"Emergency Stop", c.vkEmergencyStop);

    wchar_t tbuf[64];
    swprintf_s(tbuf, L"Color tolerance: %d", c.colorTolerance);
    SetTextColor(mem, C_TEXT);
    TextOutW(mem, 24, 364, tbuf, (int)wcslen(tbuf));
    swprintf_s(tbuf, L"Reaction ms: %d", c.reactionMs);
    TextOutW(mem, 24, 400, tbuf, (int)wcslen(tbuf));

    if (gCaptureHotkey) {
        SetTextColor(mem, C_WARN);
        SelectObject(mem, gFontUI);
        TextOutW(mem, 400, 360, L"Press a key...", 14);
    }

    // Buttons
    for (auto& b : gBtns) {
        COLORREF fill = b.hover ? RGB(18, 36, 56) : RGB(14, 22, 34);
        COLORREF border = C_BORDER;
        if (b.id == ID_ARM && c.armed) border = C_OK;
        if (b.id == ID_FAST && c.fastSniperEnabled) border = C_ACCENT;
        DrawRoundRect(mem, b.rc, fill, border, 8);
        SetTextColor(mem, C_TEXT);
        SelectObject(mem, gFontSmall);
        RECT tr = b.rc;
        DrawTextW(mem, b.label.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Footer
    SetTextColor(mem, C_MUTED);
    SelectObject(mem, gFontSmall);
    TextOutW(mem, 24, client.bottom - 28,
              L"Play smart, not hard  ·  FARSHAD PM CHEATS",
              42);

    BitBlt(hdc, 0, 0, client.right, client.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

static void HandleBtn(int id) {
    auto& c = Config();
    auto& log = Logger::Instance();
    switch (id) {
    case ID_ARM:
        c.armed = !c.armed;
        log.Info(c.armed ? L"ARMED from UI" : L"DISARMED from UI");
        c.Save();
        break;
    case ID_FAST:
        c.fastSniperEnabled = !c.fastSniperEnabled;
        log.Info(c.fastSniperEnabled ? L"Fast Sniper ON" : L"Fast Sniper OFF");
        c.Save();
        break;
    case ID_SAVE:
        c.Save();
        log.Success(L"Config saved");
        break;
    case ID_CLEARLOG:
        Logger::Instance().Clear();
        RefreshLogList();
        break;
    case ID_HK_ACTIVATE: gCaptureHotkey = 1; log.Info(L"Rebind Activate — press a key"); break;
    case ID_HK_ARMED: gCaptureHotkey = 2; log.Info(L"Rebind Armed — press a key"); break;
    case ID_HK_FAST: gCaptureHotkey = 3; log.Info(L"Rebind Fast Sniper — press a key"); break;
    case ID_HK_STOP: gCaptureHotkey = 4; log.Info(L"Rebind Emergency — press a key"); break;
    case ID_TOL_MINUS: c.colorTolerance = max(0, c.colorTolerance - 1); c.Save(); break;
    case ID_TOL_PLUS: c.colorTolerance = min(64, c.colorTolerance + 1); c.Save(); break;
    case ID_RXN_MINUS: c.reactionMs = max(0, c.reactionMs - 1); c.Save(); break;
    case ID_RXN_PLUS: c.reactionMs = min(100, c.reactionMs + 1); c.Save(); break;
    }
    InvalidateRect(gHwnd, nullptr, FALSE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        gHwnd = hwnd;
        LayoutButtons();
        gLogList = CreateWindowExW(
            0, L"LISTBOX", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
            16, 440, kW - 48, 140,
            hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        SendMessageW(gLogList, WM_SETFONT, (WPARAM)gFontSmall, TRUE);
        Logger::Instance().SetUiCallback(OnLogUi);
        Logger::Instance().Success(L"Farshad PM ready");
        Logger::Instance().Info(L"Support: @farshad_pm_org");
        RefreshLogList();
        return 0;
    }
    case WM_APP + 1:
        RefreshLogList();
        return 0;
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        bool changed = false;
        for (auto& b : gBtns) {
            bool h = PtInRect(&b.rc, POINT{ x, y });
            if (h != b.hover) { b.hover = h; changed = true; }
        }
        if (changed) InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        for (auto& b : gBtns) {
            if (PtInRect(&b.rc, POINT{ x, y })) {
                HandleBtn(b.id);
                break;
            }
        }
        return 0;
    }
    case WM_KEYDOWN: {
        if (gCaptureHotkey) {
            int vk = (int)wParam;
            auto& c = Config();
            if (gCaptureHotkey == 1) c.vkActivate = vk;
            else if (gCaptureHotkey == 2) c.vkToggleArmed = vk;
            else if (gCaptureHotkey == 3) c.vkToggleFastSniper = vk;
            else if (gCaptureHotkey == 4) c.vkEmergencyStop = vk;
            c.Save();
            Logger::Instance().Success(L"Hotkey set to " + AppConfig::KeyName(vk));
            gCaptureHotkey = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, C_TEXT);
        SetBkColor(hdc, RGB(10, 14, 22));
        static HBRUSH br = CreateSolidBrush(RGB(10, 14, 22));
        return (LRESULT)br;
    }
    case WM_DESTROY:
        if (gBot) {
            gBot->Stop();
            delete gBot;
            gBot = nullptr;
        }
        Config().Save();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nShow) {
    InitCommonControls();

    gFontTitle = CreateFontW(32, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Segoe UI");
    gFontUI = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH, L"Segoe UI");
    gFontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Consolas");
    gBrushBg = CreateSolidBrush(C_BG);
    gBrushPanel = CreateSolidBrush(C_PANEL);

    Config().Load();

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = kClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = gBrushBg;
    RegisterClassExW(&wc);

    RECT r{ 0, 0, kW, kH };
    AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND hwnd = CreateWindowExW(
        0, kClass, L"FARSHAD PM — Trigger Bot",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, hInst, nullptr);

    gBot = new BotEngine();
    gBot->Start();

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeleteObject(gFontTitle);
    DeleteObject(gFontUI);
    DeleteObject(gFontSmall);
    DeleteObject(gBrushBg);
    DeleteObject(gBrushPanel);
    return (int)msg.wParam;
}
