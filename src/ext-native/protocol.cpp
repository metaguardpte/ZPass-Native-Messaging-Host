#include <iostream>
#include "json.hpp"
#include "protocol.h"
#include "utils.h"
#include "logger.h"
#include <fcntl.h>
#include <io.h>
using namespace std;
using namespace nlohmann;

void StdIoHandler::SwitchIoMode()
{
    cout.setf(std::ios_base::unitbuf);
    _setmode(_fileno(stdin), _O_BINARY);
}

bool StdIoHandler::ParsePackage(const string& strPack, ExtensionRequest& request)
{
    auto result = false;

    try
    {
        auto jsonObj = json::parse(strPack);
        if (!jsonObj.contains("action"))
            return false;
        jsonObj.at("action").get_to(request.action);

        request.path.clear();
        if (jsonObj.contains("path"))
            jsonObj.at("path").get_to(request.path);

        request.openApp = false;
        if (jsonObj.contains("openApp"))
            jsonObj.at("openApp").get_to(request.openApp);

        request.waitApp = false;
        if (jsonObj.contains("waitApp"))
            jsonObj.at("waitApp").get_to(request.waitApp);

        request.timestamp = 0;
        if (jsonObj.contains("timestamp"))
            jsonObj.at("timestamp").get_to(request.timestamp);

        request.hash.clear();
        if (jsonObj.contains("hash"))
            jsonObj.at("hash").get_to(request.hash);

        result = true;
    }
    catch (exception)
    {
    }

    return result;
}

void StdIoHandler::Response(const std::string& msg)
{
    if (!msg.empty())
    {
        m_refLogger.Log(Level_Info, "Reponse: %s", msg.c_str());
        int size = static_cast<int>(msg.size());
        cout.write(reinterpret_cast<char*>(&size), 4);
        cout << msg;
    }
}

bool StdIoHandler::ReadPackage(std::string& strPack)
{
    const unsigned int MaxPackSize = 1024;
    unsigned int packLen = 0;
    cin.read(reinterpret_cast<char*>(&packLen), 4);
    m_refLogger.Log(Level_Info, "Read package head: %d", packLen);
    if (packLen == 0 || packLen > MaxPackSize) {
        m_refLogger.Log(Level_Warn, "IsLegal request head: %d", packLen);
    }

    for (unsigned int i = 0; i < packLen; i++) {
        auto currChar = getchar();
        if (currChar == EOF) {
            m_refLogger.Log(Level_Warn, "IsLegal request, read EOF at %d", i);
            return false;
        }
        strPack += currChar;
    }
    m_refLogger.Log(Level_Debug, "Read copmlete: %s", strPack);
    return true;
}

bool StdIoHandler::ReadRequest(ExtensionRequest& request)
{
    string strPack;
    if (!ReadPackage(strPack))
        return false;
    return ParsePackage(strPack, request);
}

bool StdIoHandler::WaitForStdInClosed(int timeoutSeconds)
{
    auto res = ::WaitForSingleObject(m_hMonitorTask, timeoutSeconds * 1000);
    if (res != WAIT_TIMEOUT) {
        m_refLogger.Log(Level_Info, "Wait app with result %d %s", res, (WAIT_OBJECT_0 == res) ? "app closed" : "");
        if (m_hMonitorTask != nullptr) {
            CloseHandle(m_hMonitorTask);
            m_hMonitorTask = nullptr;
        }
        return true;
    }
    return false;
}

void StdIoHandler::StartMonitorStdIn()
{
    if (m_hMonitorTask != nullptr)
        return;
    m_hMonitorTask = ::CreateThread(nullptr, 0, _MonitorStdIn, (LPVOID)this, 0, nullptr);
}

DWORD WINAPI StdIoHandler::_MonitorStdIn(LPVOID param)
{
    auto handler = reinterpret_cast<StdIoHandler*>(param);
    return handler->MonitorStdIn();
}

DWORD StdIoHandler::MonitorStdIn()
{
    while (true)
    {
        auto currChar = getchar();
        if (currChar == EOF) {
            m_refLogger.Log(Level_Info, "_MonitorStdIn - read EOF");
            break;
        }
        m_refLogger.Log(Level_Info, "_MonitorStdIn - read one %d", currChar);
    }
    return 0;
}

NativeResp::NativeResp(int wsPort) : NativeResp(wsPort, "success", wsPort)
{}
NativeResp::NativeResp(int errCode, std::string errMsg, int wsPort)
    : err_code(errCode), err_msg(errMsg), ws_port(wsPort)
{}

std::string NativeResp::ToJson()
{
    auto result = nlohmann::json{
        { "err_code", err_code },
        { "err_msg", err_msg },
        { "ws_port", ws_port }
    };
    return result.dump();
}

std::vector<char> CIpcRequest::GetProtocolBuffer()
{
    //Base64(json) + '\0'
    auto result = json{
        { "action", action },
        { "timestamp", timestamp },
        { "auth", auth }
    };
    auto regJson = result.dump();
    std::string base64;
    base64_encode(reinterpret_cast<unsigned char const*>(regJson.data()), regJson.size(), base64);
    auto cstr = base64.c_str();
    return std::vector<char>(cstr, cstr + base64.size() + 1);
}
