#include "Bot.h"
#include "Config.h"
#include "Logger.h"
#include <cmath>
#include <algorithm>

BotEngine::BotEngine() {
    screen_ = GetDC(nullptr);
    RECT r{};
    GetWindowRect(GetDesktopWindow(), &r);
    cx_ = (r.right - r.left) / 2;
    cy_ = (r.bottom - r.top) / 2;
}

BotEngine::~BotEngine() {
    Stop();
    if (screen_) {
        ReleaseDC(nullptr, screen_);
        screen_ = nullptr;
    }
}

bool BotEngine::KeyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

int BotEngine::ColorDist(COLORREF a, COLORREF b) {
    int dr = GetRValue(a) - GetRValue(b);
    int dg = GetGValue(a) - GetGValue(b);
    int db = GetBValue(a) - GetBValue(b);
    return (int)std::sqrt((double)(dr * dr + dg * dg + db * db));
}

void BotEngine::LeftClick() {
    INPUT in[2]{};
    in[0].type = INPUT_MOUSE;
    in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    in[1].type = INPUT_MOUSE;
    in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, in, sizeof(INPUT));
}

void BotEngine::TapKey(int vk) {
    INPUT in[2]{};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = (WORD)vk;
    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = (WORD)vk;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
}

void BotEngine::FireFastSniper() {
    auto& c = Config();
    TapKey('Q');
    Sleep((DWORD)(std::max)(1, c.fastSniperGapMs));
    TapKey('Q');
    Logger::Instance().Success(L"Fast Sniper: Q + Q");
}

void BotEngine::Start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread([this] { Loop(); });
    Logger::Instance().Success(L"Bot engine started");
}

void BotEngine::Stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
    Logger::Instance().Warn(L"Bot engine stopped");
}

void BotEngine::Loop() {
    auto& log = Logger::Instance();
    DWORD lastClick = 0;
    bool prevArmedKey = false;
    bool prevFsKey = false;

    while (running_) {
        auto& c = Config();

        if (KeyDown(c.vkEmergencyStop)) {
            c.armed = false;
            c.fastSniperEnabled = false;
            log.Error(L"EMERGENCY STOP");
            Sleep(300);
            continue;
        }

        bool armedKey = KeyDown(c.vkToggleArmed);
        if (armedKey && !prevArmedKey) {
            c.armed = !c.armed;
            log.Info(c.armed ? L"ARMED" : L"DISARMED");
            c.Save();
        }
        prevArmedKey = armedKey;

        bool fsKey = KeyDown(c.vkToggleFastSniper);
        if (fsKey && !prevFsKey) {
            c.fastSniperEnabled = !c.fastSniperEnabled;
            log.Info(c.fastSniperEnabled ? L"Fast Sniper ON" : L"Fast Sniper OFF");
            c.Save();
        }
        prevFsKey = fsKey;

        if (!c.armed) {
            Sleep(15);
            continue;
        }

        if (KeyDown(c.vkActivate)) {
            COLORREF base = GetPixel(screen_, cx_, cy_);
            Sleep(2);
            while (running_ && KeyDown(c.vkActivate) && c.armed) {
                COLORREF now = GetPixel(screen_, cx_, cy_);
                if (ColorDist(base, now) > c.colorTolerance) {
                    DWORD t = GetTickCount();
                    if (t - lastClick >= (DWORD)c.cooldownMs) {
                        if (c.reactionMs > 0) Sleep((DWORD)c.reactionMs);
                        LeftClick();
                        lastClick = GetTickCount();
                        triggers_++;
                        log.Debug(L"Trigger click");

                        if (c.fastSniperEnabled) {
                            FireFastSniper();
                        }

                        int guard = 0;
                        while (running_ && KeyDown(c.vkActivate) && guard++ < 200) {
                            if (ColorDist(base, GetPixel(screen_, cx_, cy_)) <= c.colorTolerance)
                                break;
                            Sleep(5);
                        }
                        base = GetPixel(screen_, cx_, cy_);
                    }
                }
                Sleep(1);
            }
        } else {
            Sleep(5);
        }
    }
}
