#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <ctime>
#include <Windows.h>
#include <shlobj_core.h>
#include "product.h"
#include "logger.h"
#include "product.h"
#include <direct.h>
#include <system_error>
#include <cstdarg>
#include "utils.h"
using namespace std;

CLogger::CLogger()
{
    m_logLevel = LogLevel::Level_Warn;
    m_vBuffer.resize(2048);
    char szBuffer[64] = { 0 };
    _snprintf_s(szBuffer, sizeof(szBuffer) - 1, " PID[%u] - ", GetCurrentProcessId());
    m_strHeadInfo.assign(szBuffer);
}

CLogger::~CLogger()
{
    if (m_logger.is_open()) m_logger.close();
}

void CLogger::Log(LogLevel level, char const* const format, ...)
{
    if (level < m_logLevel)
        return;
    va_list va;
    va_start(va, format);
    auto buffer = m_vBuffer.data();
    int nWrite = vsnprintf(buffer, m_vBuffer.size(), format, va);
    va_end(va);

    if (nWrite < 0)
        nWrite = 0;
    else if ((size_t)nWrite >= m_vBuffer.size() - 1)
        nWrite = m_vBuffer.size() - 1;
    buffer[nWrite++] = '\n';

    string strWrite;
    strWrite.assign(currenttime()).append(m_strHeadInfo).append(buffer, nWrite);
    m_logger.write(strWrite.data(), strWrite.size());
    m_logger.flush();
}

bool CLogger::Init()
{
    if (m_logger.is_open())
        return true;
    auto ret = GetModuleFileNameA(nullptr, m_vBuffer.data(), m_vBuffer.size() - 1);
    string strExeFile(m_vBuffer.data());
    m_strLogName = strExeFile.substr(strExeFile.find_last_of('\\') + 1) + ".log";

    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, m_vBuffer.data())))
    {
        m_strLogPath.assign(m_vBuffer.data()).append("\\").append(Hubstudio::ProdName).append("\\logs");
        if (!filesystem::exists(m_strLogPath) && !filesystem::create_directories(m_strLogPath)) {
            return false;
        }
    }
    else {
        m_strLogPath = strExeFile.substr(0, strExeFile.find_last_of('\\'));
    }

    string strLogFullName;
    strLogFullName.assign(m_strLogPath).append("\\").append(m_strLogName);
    m_logger.open(strLogFullName.c_str(), std::ios::app);
    return m_logger.is_open();
}
