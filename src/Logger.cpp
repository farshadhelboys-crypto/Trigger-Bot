#include "Logger.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

Logger& Logger::Instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() {
    try {
        wchar_t path[MAX_PATH]{};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
            fs::path dir = fs::path(path) / L"FarshadPM" / L"logs";
            fs::create_directories(dir);
            file_.open(dir / L"farshad_pm.log", std::ios::app);
        }
    } catch (...) {
    }
    // Fallback: local logs folder
    if (!file_.is_open()) {
        try {
            fs::create_directories(L"logs");
            file_.open(L"logs/farshad_pm.log", std::ios::app);
        } catch (...) {
        }
    }
}

// Need shell path — include after to avoid circular issues in some toolchains
#include <shlobj.h>

std::wstring Logger::Now() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[32];
    swprintf_s(buf, L"%02d:%02d:%02d.%03d",
               st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

const wchar_t* Logger::LevelTag(LogLevel l) {
    switch (l) {
    case LogLevel::Info: return L"INFO";
    case LogLevel::Warn: return L"WARN";
    case LogLevel::Error: return L"ERROR";
    case LogLevel::Success: return L"OK";
    case LogLevel::Debug: return L"DBG";
    }
    return L"?";
}

void Logger::Push(LogLevel level, const std::wstring& msg) {
    LogEntry e{ Now(), level, msg };
    {
        std::lock_guard<std::mutex> lock(mtx_);
        entries_.push_back(e);
        while (entries_.size() > kMaxEntries) entries_.pop_front();
        if (file_.is_open()) {
            file_ << L"[" << e.time << L"] [" << LevelTag(level) << L"] " << msg << L"\n";
            file_.flush();
        }
    }
    if (uiCb_) uiCb_();
}

void Logger::Info(const std::wstring& m) { Push(LogLevel::Info, m); }
void Logger::Warn(const std::wstring& m) { Push(LogLevel::Warn, m); }
void Logger::Error(const std::wstring& m) { Push(LogLevel::Error, m); }
void Logger::Success(const std::wstring& m) { Push(LogLevel::Success, m); }
void Logger::Debug(const std::wstring& m) { Push(LogLevel::Debug, m); }

std::vector<LogEntry> Logger::Snapshot() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return { entries_.begin(), entries_.end() };
}

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    entries_.clear();
}

void Logger::SetUiCallback(void (*cb)()) { uiCb_ = cb; }
