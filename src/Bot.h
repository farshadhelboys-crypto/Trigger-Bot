#pragma once
#include <Windows.h>
#include <atomic>
#include <thread>

class BotEngine {
public:
    BotEngine();
    ~BotEngine();

    void Start();
    void Stop();

    bool IsRunning() const { return running_.load(); }
    unsigned long long TriggerCount() const { return triggers_.load(); }

    // Fast sniper: press Q twice within configured window
    void FireFastSniper();

private:
    void Loop();
    static bool KeyDown(int vk);
    static void LeftClick();
    static void TapKey(int vk);
    static int ColorDist(COLORREF a, COLORREF b);

    HDC screen_ = nullptr;
    int cx_ = 0, cy_ = 0;
    std::atomic<bool> running_{ false };
    std::atomic<unsigned long long> triggers_{ 0 };
    std::thread worker_;
};
