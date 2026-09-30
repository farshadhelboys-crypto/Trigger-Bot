#include "Config.h"
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

static fs::path ConfigPath() {
    wchar_t path[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        fs::path dir = fs::path(path) / L"FarshadPM";
        fs::create_directories(dir);
        return dir / L"config.ini";
    }
    return L"config.ini";
}

AppConfig& Config() {
    static AppConfig c;
    return c;
}

static void WriteKV(std::wofstream& f, const wchar_t* k, int v) {
    f << k << L"=" << v << L"\n";
}

void AppConfig::Save() const {
    try {
        std::wofstream f(ConfigPath());
        if (!f) return;
        WriteKV(f, L"vkActivate", vkActivate);
        WriteKV(f, L"vkToggleArmed", vkToggleArmed);
        WriteKV(f, L"vkToggleFastSniper", vkToggleFastSniper);
        WriteKV(f, L"vkEmergencyStop", vkEmergencyStop);
        WriteKV(f, L"colorTolerance", colorTolerance);
        WriteKV(f, L"reactionMs", reactionMs);
        WriteKV(f, L"cooldownMs", cooldownMs);
        WriteKV(f, L"fastSniperWindowMs", fastSniperWindowMs);
        WriteKV(f, L"fastSniperGapMs", fastSniperGapMs);
        WriteKV(f, L"fastSniperEnabled", fastSniperEnabled ? 1 : 0);
    } catch (...) {
    }
}

void AppConfig::Load() {
    try {
        std::wifstream f(ConfigPath());
        if (!f) return;
        std::wstring line;
        while (std::getline(f, line)) {
            auto eq = line.find(L'=');
            if (eq == std::wstring::npos) continue;
            auto key = line.substr(0, eq);
            int val = _wtoi(line.substr(eq + 1).c_str());
            if (key == L"vkActivate") vkActivate = val;
            else if (key == L"vkToggleArmed") vkToggleArmed = val;
            else if (key == L"vkToggleFastSniper") vkToggleFastSniper = val;
            else if (key == L"vkEmergencyStop") vkEmergencyStop = val;
            else if (key == L"colorTolerance") colorTolerance = val;
            else if (key == L"reactionMs") reactionMs = val;
            else if (key == L"cooldownMs") cooldownMs = val;
            else if (key == L"fastSniperWindowMs") fastSniperWindowMs = val;
            else if (key == L"fastSniperGapMs") fastSniperGapMs = val;
            else if (key == L"fastSniperEnabled") fastSniperEnabled = val != 0;
        }
    } catch (...) {
    }
}

std::wstring AppConfig::KeyName(int vk) {
    if (vk <= 0) return L"—";
    UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    wchar_t name[64]{};
    LONG lParam = (scan << 16);
    if (GetKeyNameTextW(lParam, name, 64) > 0) return name;
    wchar_t buf[16];
    swprintf_s(buf, L"VK_%02X", vk);
    return buf;
}
