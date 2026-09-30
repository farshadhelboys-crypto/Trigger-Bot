#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <deque>

enum class LogLevel { Info, Warn, Error, Success, Debug };

struct LogEntry {
    std::wstring time;
    LogLevel level;
    std::wstring message;
};

class Logger {
public:
    static Logger& Instance();

    void Info(const std::wstring& msg);
    void Warn(const std::wstring& msg);
    void Error(const std::wstring& msg);
    void Success(const std::wstring& msg);
    void Debug(const std::wstring& msg);

    std::vector<LogEntry> Snapshot() const;
    void Clear();
    void SetUiCallback(void (*cb)());

private:
    Logger();
    void Push(LogLevel level, const std::wstring& msg);
    static std::wstring Now();
    static const wchar_t* LevelTag(LogLevel l);

    mutable std::mutex mtx_;
    std::deque<LogEntry> entries_;
    std::wofstream file_;
    void (*uiCb_)() = nullptr;
    static constexpr size_t kMaxEntries = 500;
};
