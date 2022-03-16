#include <windows.h>
#include <string>

class CLogger;

typedef struct {
    std::string action;  //get-wss
    std::string path;
    bool openApp;
    bool waitApp;
    long timestamp;   //authenticate
    std::string hash; //authenticate
}ExtensionRequest;

class StdIoHandler
{
    CLogger& m_refLogger;
    HANDLE m_hMonitorTask;
public:
    StdIoHandler(CLogger& logger) : m_refLogger(logger), m_hMonitorTask(nullptr)
    {
        SwitchIoMode();
    }
    bool ReadRequest(ExtensionRequest& request);
    void Response(const std::string& msg);
    void StartMonitorStdIn();
    bool WaitForStdInClosed(int timeoutSeconds = 1);
private:
    void SwitchIoMode();
    bool ReadPackage(std::string& strPack);
    bool ParsePackage(const std::string& strPack, ExtensionRequest& request);
    DWORD MonitorStdIn();
    static DWORD WINAPI _MonitorStdIn(LPVOID param);
};

class CIpcRequest
{
public:
    std::string action;
    time_t timestamp;
    std::string auth;

    std::vector<char> GetProtocolBuffer();
};

class NativeResp
{
public:
    enum 
    {
        Resp_Success = 0,
        Resp_NotRunning = 1,
        Resp_StartApp = 3,
        Resp_WaitAppService = 4,
        Resp_NotInstalled = 5,
        Resp_InnerError = 10,
    };
    int err_code;
    int ws_port;
    std::string err_msg;
    NativeResp() = delete;
    NativeResp(int wsPort);
    NativeResp(int errCode, std::string errMsg, int wsPort = 0);
    std::string ToJson();
};


