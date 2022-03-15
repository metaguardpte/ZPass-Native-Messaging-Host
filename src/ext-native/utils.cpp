#include "utils.h"
#include <Windows.h>
#include <time.h>
#include <vector>

using namespace std;

const int InvalidPosition = -1;
const char* base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
"+/";

static int pos_of_char(const unsigned char chr) {
    if (chr >= 'A' && chr <= 'Z') return chr - 'A';
    else if (chr >= 'a' && chr <= 'z') return chr - 'a' + ('Z' - 'A') + 1;
    else if (chr >= '0' && chr <= '9') return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    else if (chr == '+' || chr == '-') return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
    else if (chr == '/' || chr == '_') return 63; // Ditto for '/' and '_'
    else return InvalidPosition;
}

void base64_encode(unsigned char const* bytes_to_encode, size_t in_len, std::string& ret) {

    size_t len_encoded = (in_len + 2) / 3 * 4;
    unsigned char trailing_char = '=';
    ret.reserve(len_encoded);

    unsigned int pos = 0;

    while (pos < in_len) {
        ret.push_back(base64_chars[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);

        if (pos + 1 < in_len) {
            ret.push_back(base64_chars[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

            if (pos + 2 < in_len) {
                ret.push_back(base64_chars[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
                ret.push_back(base64_chars[bytes_to_encode[pos + 2] & 0x3f]);
            }
            else {
                ret.push_back(base64_chars[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
                ret.push_back('=');
            }
        }
        else {

            ret.push_back(base64_chars[(bytes_to_encode[pos + 0] & 0x03) << 4]);
            ret.push_back('=');
            ret.push_back('=');
        }
        pos += 3;
    }
}


bool base64_decode(const string& encoded_string, string& out) {

    out.clear();
    if (encoded_string.empty()) return true;

    size_t length_of_string = encoded_string.length();
    size_t pos = 0;
    size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
    out.reserve(approx_length_of_decoded_string);

    while (pos < length_of_string) {
        auto pos_of_char_0 = pos_of_char(encoded_string[pos + 0]);
        auto pos_of_char_1 = pos_of_char(encoded_string[pos + 1]);
        if (pos_of_char_0 == InvalidPosition || pos_of_char_1 == InvalidPosition) return false;
        out.push_back(static_cast<std::string::value_type>((pos_of_char_0 << 2) + ((pos_of_char_1 & 0x30) >> 4)));
        if ((pos + 2 < length_of_string) && encoded_string[pos + 2] != '=')
        {
            unsigned int pos_of_char_2 = pos_of_char(encoded_string[pos + 2]);
            if (pos_of_char_1 == InvalidPosition) return false;
            out.push_back(static_cast<std::string::value_type>(((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2)));
            if ((pos + 3 < length_of_string) && encoded_string[pos + 3] != '=')
            {
                unsigned int pos_of_char_3 = pos_of_char(encoded_string[pos + 3]);
                if (pos_of_char_3 == InvalidPosition) return false;
                out.push_back(static_cast<std::string::value_type>(((pos_of_char_2 & 0x03) << 6) + pos_of_char_3));
            }
        }
        pos += 4;
    }

    return true;
}

static LONG GetStringRegKey(HKEY hKey, const std::string& strValueName, std::string& strValue)
{
    char szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExA(hKey, strValueName.c_str(), 0, nullptr, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    return nError;
}

static int ReadProductFilePath(HKEY hRoot, LPCSTR prodRegKey, LPCSTR prodExeName, string& path)
{
    string strAppRegKey;
    strAppRegKey.assign("SOFTWARE\\").append(prodRegKey);
    HKEY hKey;
    LONG nError = RegOpenKeyExA(hRoot, strAppRegKey.c_str(), 0, KEY_READ, &hKey);
    if (nError != ERROR_SUCCESS) return nError;
    nError = GetStringRegKey(hKey, "InstallLocation", path);
    RegCloseKey(hKey);
    if (nError == ERROR_SUCCESS)
        path.append("\\").append(prodExeName);
    return nError;
}


static bool CheckFileAccess(const string& path)
{
    bool state = true;
    DWORD attrib = GetFileAttributesA(path.c_str());
    if (attrib == INVALID_FILE_ATTRIBUTES)
    {
        state = false;      // not accessible
    }
    else if (attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        state = false;      // it is a directory
    }
    return state;
}

bool IsProductInstalled(const char* prodRegKey, const char* prodExeName, std::string& prodPath)
{
    LONG nError = ReadProductFilePath(HKEY_CURRENT_USER, prodRegKey, prodExeName, prodPath);
    if (nError == ERROR_SUCCESS && CheckFileAccess(prodPath.c_str()))
        return true;
    nError = ReadProductFilePath(HKEY_LOCAL_MACHINE, prodRegKey, prodExeName, prodPath);
    if (nError == ERROR_SUCCESS && CheckFileAccess(prodPath.c_str()))
        return true;
    return false;
}

const char* currenttime()
{
    time_t rawtime;
    struct tm timeinfo;
    static char buffer[80];
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%d-%m %H:%M:%S", &timeinfo);
    return buffer;
}

bool StartApplication(const std::string& path)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process.
    vector<char> vBuffer;
    vBuffer.resize(path.size() + 1);
    memcpy(vBuffer.data(), path.c_str(), path.size() + 1);
    
    if (!CreateProcessA(nullptr,    // No module name (use command line)
        vBuffer.data(),        // Command line
        nullptr,           // Process handle not inheritable
        nullptr,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        nullptr,           // Use parent's environment block
        nullptr,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        )
    {
        //SetLastError("CreateProcess faild");
        return false;
    }

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}
