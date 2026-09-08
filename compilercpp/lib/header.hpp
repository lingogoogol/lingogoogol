#ifndef COMPILERCPP_LIB_HEADER
#define COMPILERCPP_LIB_HEADER

#include <windowsx.h>
#include <wrl/client.h>
#include <d2d1_3.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <dwrite_3.h>
#include <dxgi1_6.h>
#include <shellapi.h>
#include <wincrypt.h>
#include <ShObjIdl.h>
#include <objbase.h>

#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/decoder.h>
#include <nlohmann/json.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#undef min
#undef max
#undef WIN32_LEAN_AND_MEAN
#undef near
#undef far
#undef interface

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Ole32.lib")

#endif
