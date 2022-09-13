#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include "json.hpp"
#include "pipe.h"
#include "product.h"
#include "logger.h"
#include "protocol.h"
#include "utils.h"
using json = nlohmann::json;
using namespace std;

bool LoadExtNativeConfig(vector<string>& vAllowedExtension);
bool ParseCmdline(int argc, char* argv[]);
string HandleRequest(StdIoHandler& stdIoHandler, const ExtensionRequest& extReq);

std::vector<string> g_vAllowedExtensions;
CLogger logger;
struct
{
    HWND hwndBrowser;
    LPCSTR extensionId;
} CmdlineInfo;

int main(int argc, char* argv[])
{
    const int ExitCode_InvalidRequest = 886;

    logger.Init();

    if (!LoadExtNativeConfig(g_vAllowedExtensions)) {
        logger.Log(Level_Warn, "Fail to load configuration");
        return ExitCode_InvalidRequest;
    }

    if (!ParseCmdline(argc, argv))
        return ExitCode_InvalidRequest;
    logger.Log(LogLevel::Level_Info, "current extension Id: %s", CmdlineInfo.extensionId);

    ExtensionRequest request;
    StdIoHandler stdIoHandler(logger);
    if (!stdIoHandler.ReadRequest(request)) {
        logger.Log(Level_Info, "Invalid request");
        return ExitCode_InvalidRequest;
    }

    if (strcmp(request.action.c_str(), "get-wss") != 0) {
        logger.Log(Level_Warn, "Failed to verify the request");
        return ExitCode_InvalidRequest;
    }

    auto reponse = HandleRequest(stdIoHandler, request);
    stdIoHandler.Response(reponse);

    return 0;
}

string GetWsPort(CPipeClient& client)
{
    CIpcRequest req;
    req.action = "get-wss";
    req.timestamp = time(nullptr);
    auto reqData = req.GetProtocolBuffer();
    logger.Log(Level_Info, "Send request");
    if (!client.WriteData(reqData.data(), reqData.size()))
        return NativeResp(NativeResp::Resp_InnerError, client.LastError()).ToJson();

    string ipcResp;
    logger.Log(Level_Info, "Read reponse");
    if (!client.ReadData(ipcResp))
        return NativeResp(NativeResp::Resp_InnerError, client.LastError()).ToJson();
    return NativeResp(atoi(ipcResp.c_str())).ToJson();
}

string HandleRequest(StdIoHandler& stdIoHandler, const ExtensionRequest& extReq)
{
    string appPath;
    if (!IsProductInstalled(Hubstudio::ProdRegistryName, Hubstudio::ProdExeName, appPath)) {
        logger.Log(Level_Debug, "Product is not installed yet.");
        return NativeResp(NativeResp::Resp_NotInstalled, "Product is not installed yet.").ToJson();
    }

    CPipeClient client(logger);

    //App is running
    logger.Log(Level_Info, "Try connect to ipc service...");
    if (client.ConnectServer(Hubstudio::ProductPipeName)) {
        return GetWsPort(client);
    }

    //Open App
    if (extReq.openApp) {
        logger.Log(Level_Info, "Require starting APP...", appPath.c_str());
        if (!StartApplication(appPath))
            return NativeResp(NativeResp::Resp_StartApp, "Failed to start app").ToJson();

        //Wait 5 seconds for ipc service
        int nSecondsWait = 10;
        logger.Log(Level_Info, "Connect service ...");
        while (nSecondsWait-- > 0)
        {
            if (client.ConnectServer(Hubstudio::ProductPipeName))
                return GetWsPort(client);
            logger.Log(Level_Debug, "Failed to connect service", client.LastError());
            ::Sleep(1 * 1000);
        }
        return NativeResp(NativeResp::Resp_WaitAppService, "Failed to connec service").ToJson();
    }

    if (extReq.waitApp) {
        logger.Log(Level_Info, "Wait custom starting APP...");
        stdIoHandler.WaitForStdInClosed();
        do {
            if (stdIoHandler.WaitForStdInClosed()) {
                logger.Log(Level_Info, "The invoker closed.");
                return string();
            }
        } while (!client.ConnectServer(Hubstudio::ProductPipeName));
        logger.Log(Level_Info, "the app is running...");
        return GetWsPort(client);
    }
    return NativeResp(NativeResp::Resp_NotRunning, "Product is not running").ToJson();
}

bool LoadExtNativeConfig(vector<string>& vAllowedExtension)
{
    auto result = false;

    vector<char> vBuffer;
    vBuffer.resize(1024);
    ifstream manifest;
    manifest.open("manifest.json", ios::in);
    if (!manifest) {
        strerror_s(vBuffer.data(), vBuffer.size() - 1, errno);
        std::cerr << "unable to open the 'manifest.json' with error" << vBuffer.data();
        return result;
    }
    manifest.read(vBuffer.data(), vBuffer.size() - 1);
    try
    {
        auto jsonObj = json::parse(vBuffer.data());

        auto loglevel = LogLevel::Level_Warn;
        if (jsonObj.contains("loglevel")) {
            auto level = jsonObj.at("loglevel").get<string>();
            if (_stricmp(level.c_str(), "debug") == 0)     loglevel = LogLevel::Level_Debug;
            else if (_stricmp(level.c_str(), "warn") == 0) loglevel = LogLevel::Level_Warn;
            else if (_stricmp(level.c_str(), "info") == 0) loglevel = LogLevel::Level_Info;
        }
        logger.SetLogLevel(loglevel);

        if (!jsonObj.contains("allowed_origins")) {
            std::cerr << "Invalid manifest.json";
        }
        else
        {
            auto& extensions = jsonObj["allowed_origins"];
            for (auto& extension : extensions)
                vAllowedExtension.push_back(extension.get<string>());
            result = true;
        }
    }
    catch (std::exception ex)
    {
        std::cerr << "Unable to manifest json: " << ex.what();
    }

    return result;
}

bool ParseCmdline(int argc, char* argv[])
{
    for (int i = 0; i < argc; ++i)
        logger.Log(Level_Info, "Start argv[%d]: %s", i, argv[i]);

    const int CmdlineParameterCount = 3;
    if (argc < CmdlineParameterCount)
        return false;

    auto paramOfParentWnd = argv[2];
    const string paramWndPrefix("--parent-window=");
    if (strncmp(paramWndPrefix.c_str(), paramOfParentWnd, paramWndPrefix.size())){
        logger.Log(Level_Info, "Invalid parameter");
        return false;
    }

    CmdlineInfo.hwndBrowser = reinterpret_cast<HWND>(atoi(paramOfParentWnd + paramWndPrefix.size()));

    LPCSTR currentExtension = argv[1];
    for (const auto& extId : g_vAllowedExtensions)
    {
        if (_strcmpi(extId.c_str(), currentExtension) == 0) {
            CmdlineInfo.extensionId = extId.c_str();
            return true;
        }
    }
    logger.Log(Level_Warn, "Unsupported extension: %s", currentExtension);

    return false;
}

