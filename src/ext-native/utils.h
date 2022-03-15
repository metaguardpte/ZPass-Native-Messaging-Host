#pragma once

#include <string>
void base64_encode(unsigned char const* bytes_to_encode, size_t in_len, std::string& out);
bool base64_decode(const std::string& src, std::string& dest);
bool IsProductInstalled(const char* prodRegKey, const char* prodExeName, std::string& installPath);
const char* currenttime();
bool StartApplication(const std::string& path);
