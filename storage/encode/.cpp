module storage.encode;

import std;

import external.Windows;
import external.wincrypt;

auto base64url_encode(std::string in) -> std::string
{
    DWORD out_size{};
    CryptBinaryToStringA
    (
        reinterpret_cast<const BYTE*>(in.data()),
        static_cast<DWORD>(in.size()),
        CRYPT_STRING_BASE64URI | CRYPT_STRING_NOCRLF,
        nullptr,
        &out_size
    );
    std::string out{};
    out.resize(out_size);
    CryptBinaryToStringA
    (
        reinterpret_cast<const BYTE*>(in.data()),
        static_cast<DWORD>(in.size()),
        CRYPT_STRING_BASE64URI | CRYPT_STRING_NOCRLF,
        out.data(),
        &out_size
    );
    out.pop_back();
    return out;
}

auto base64url_decode(std::string in) -> std::string
{
    DWORD out_size{};
    CryptStringToBinaryA
    (
        reinterpret_cast<LPCSTR>(in.data()),
        static_cast<DWORD>(in.size()),
        CRYPT_STRING_BASE64,
        nullptr,
        &out_size,
        0,
        nullptr
    );
    std::string out{};
    out.resize(out_size);
    CryptStringToBinaryA
    (
        reinterpret_cast<LPCSTR>(in.data()),
        static_cast<DWORD>(in.size()),
        CRYPT_STRING_BASE64,
        reinterpret_cast<BYTE*>(out.data()),
        &out_size,
        0,
        nullptr
    );
    return out;
}
