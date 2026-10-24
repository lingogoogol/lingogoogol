module storage.encode;

import std;

import external.Windows;
import external.wincrypt;

auto base64url_encode(std::string in) -> std::string
{
    //`CryptBinaryToStringA` fails when input length is 0.
    if (in.empty())
    {
        return {};
    }
    DWORD out_size{};
    DWORD flags{ CRYPT_STRING_BASE64URI | CRYPT_STRING_NOCRLF };
    //Wine's `crypt32.dll` does not implement `CRYPT_STRING_BASE64URI` (returns `ERROR_INVALID_PARAMETER`),
    //so fall back to standard `CRYPT_STRING_BASE64` and convert `+`/`/` to `-`/`_` manually below.
    if
    (
        !CryptBinaryToStringA
        (
            reinterpret_cast<const BYTE*>(in.data()),
            static_cast<DWORD>(in.size()),
            flags,
            nullptr,
            &out_size
        )
    )
    {
        flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;
        CryptBinaryToStringA
        (
            reinterpret_cast<const BYTE*>(in.data()),
            static_cast<DWORD>(in.size()),
            flags,
            nullptr,
            &out_size
        );
    }
    std::string out{};
    out.resize(out_size);
    CryptBinaryToStringA
    (
        reinterpret_cast<const BYTE*>(in.data()),
        static_cast<DWORD>(in.size()),
        flags,
        out.data(),
        &out_size
    );
    // The size returned by the first `CryptBinaryToStringA` call includes the terminating null character.
    if (!out.empty() && out.back() == '\0')
    {
        out.pop_back();
    }
    if (flags == (CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF))
    {
        for (char& c : out)
        {
            if (c == '+')
            {
                c = '-';
            }
            else if (c == '/')
            {
                c = '_';
            }
        }
    }
    return out;
}

auto base64url_decode(std::string in) -> std::string
{
    //Convert Base64URL (`-`, `_`) back to standard Base64 (`+`, `/`) and decode with `CRYPT_STRING_BASE64`
    //because Wine's `crypt32.dll` does not support `CRYPT_STRING_BASE64URI`.
    for (char& c : in)
    {
        if (c == '-')
        {
            c = '+';
        }
        else if (c == '_')
        {
            c = '/';
        }
    }
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
