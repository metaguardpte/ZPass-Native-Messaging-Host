namespace utils
{
    const int InvalidPosition = -1;
    void base64_encode(unsigned char const* bytes_to_encode, size_t in_len, std::string& out);
    bool base64_decode(const std::string& src, std::string& dest);
}

