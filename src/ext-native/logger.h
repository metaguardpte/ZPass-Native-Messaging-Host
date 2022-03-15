#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <memory>

enum LogLevel {
    Level_Debug = 0,
    Level_Info = 1,
    Level_Warn = 2
};

class CLogger
{
    std::string m_strLogName;
    std::string m_strLogPath;
    std::ofstream m_logger;
    std::vector<char> m_vBuffer;
    std::string m_strHeadInfo;
    LogLevel m_logLevel;
public:
    CLogger();
    ~CLogger();
    bool Init();
    void SetLogLevel(LogLevel level) { m_logLevel = level; }
    void Log(LogLevel level, char const* const format, ...);
};
