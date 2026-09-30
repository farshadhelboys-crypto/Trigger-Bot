#pragma once
#include <Windows.h>
#include <string>

struct AppConfig {
    // Virtual-key codes
    int vkActivate = 0x54;      // T — hold to run trigger
    int vkToggleArmed = 0x75;   // F6
    int vkToggleFastSniper = 0x76; // F7
    int vkEmergencyStop = 0x23; // END

    bool armed = false;
    bool fastSniperEnabled = false;

    // Trigger sensitivity
    int colorTolerance = 12;    // 0-255 RGB distance
    int reactionMs = 8;         // delay before click
    int cooldownMs = 40;        // min time between clicks

    // Fast sniper: two Q presses inside this window
    int fastSniperWindowMs = 500;
    int fastSniperGapMs = 40;   // gap between the two Q presses

    void Load();
    void Save() const;

    static std::wstring KeyName(int vk);
};

AppConfig& Config();
