#pragma once
#include <windows.h>
class CLogger;
class CPipeClient
{
    HANDLE m_hPipe;
    std::string m_strLastError;
    CLogger& m_refLoger;
public:
    CPipeClient(CLogger& loger) : m_refLoger(loger), m_hPipe(nullptr) {}
    ~CPipeClient() { if (m_hPipe) CloseHandle(m_hPipe); }

    bool ConnectServer(LPCSTR serverPipeName);
    bool WriteData(const void* pData, unsigned int size);
    bool ReadData(std::string& data);
    LPCSTR LastError() { return m_strLastError.c_str(); };
private:
    void SetLastError(LPCSTR desc);
};
