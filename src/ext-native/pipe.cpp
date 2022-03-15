#include <string>
#include "json.hpp"
#include <Windows.h>
#include "error-code.h"
#include "utils.h"
#include "pipe.h"
#include <stdlib.h>
#include "logger.h"

using namespace std;

void CPipeClient::SetLastError(LPCSTR desc)
{
    char szErrorCode[16] = { 0 };
    _itoa_s(::GetLastError(), szErrorCode, sizeof(szErrorCode), 10);
    m_strLastError.assign(desc).append(", error code: ").append(szErrorCode);
}

bool CPipeClient::ConnectServer(LPCSTR serverPipeName)
{
    if (!m_hPipe) {
        std::string pipeName("\\\\.\\pipe\\");
        pipeName.append(serverPipeName);
        m_hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (m_hPipe == INVALID_HANDLE_VALUE) {
            SetLastError(GetLastError() != ERROR_PIPE_BUSY ? "Could not open pipe" : "The pipe is busy");
            CloseHandle(m_hPipe);
            m_hPipe = nullptr;
        }
    }
    return (m_hPipe != nullptr);
}

bool CPipeClient::WriteData(const void* pData, unsigned int size)
{
    DWORD dwMode = PIPE_READMODE_BYTE;
    auto fSuccess = SetNamedPipeHandleState(m_hPipe, &dwMode, nullptr, nullptr);
    if (!fSuccess)
    {
        SetLastError("SetNamedPipeHandleState failed");
        return false;
    }

    DWORD dwWritten = 0;
    fSuccess = WriteFile(m_hPipe, pData, size, &dwWritten, nullptr);
    if (!fSuccess) {
        SetLastError("WriteFile to pipe failed");
        return false;
    }
    return true;
}

bool CPipeClient::ReadData(std::string& data)
{
    char szBuffer[512];
    BOOL fSuccess;
    DWORD cbRead;
    do {
        fSuccess = ReadFile(m_hPipe, szBuffer, sizeof(szBuffer), &cbRead, nullptr);
        if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
            SetLastError("ReadFile from pipe failed");
            return false;
        }
        data.append(szBuffer, cbRead);
    } while ( ! fSuccess);

    return true;
}
