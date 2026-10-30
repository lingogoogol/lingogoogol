// Test-only software KSP for Wine, whose NCrypt ECDSA/OAEP operations are absent.
// Never linked into storage.exe. All fixture keys live in a temporary directory.
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/core_names.h>
#include <cstdio>
#include <cstring>
#include <cwchar>

struct fixture_key_t { EVP_PKEY* key{}; char path[512]{}; bool signing{}; };
static auto fixture_path(fixture_key_t* key,LPCWSTR name) -> void
{
    std::snprintf(key->path,sizeof(key->path),"fixture-key-%ls.pem",name);
}
static auto fixture_provider(NCRYPT_PROV_HANDLE* out,LPCWSTR,DWORD) -> SECURITY_STATUS
{ *out=1; return 0; }
static auto fixture_open(NCRYPT_PROV_HANDLE,NCRYPT_KEY_HANDLE* out,LPCWSTR name,DWORD,DWORD) -> SECURITY_STATUS
{
    auto key=new fixture_key_t;
    fixture_path(key,name);
    auto file=std::fopen(key->path,"rb");
    if (!file) { delete key; return NTE_BAD_KEYSET; }
    key->key=PEM_read_PrivateKey(file,nullptr,nullptr,nullptr);
    std::fclose(file);
    if (!key->key) { delete key; return NTE_BAD_KEYSET; }
    key->signing=EVP_PKEY_is_a(key->key,"EC");
    *out=reinterpret_cast<NCRYPT_KEY_HANDLE>(key); return 0;
}
static auto fixture_create(NCRYPT_PROV_HANDLE,NCRYPT_KEY_HANDLE* out,LPCWSTR algorithm,LPCWSTR name,DWORD,DWORD) -> SECURITY_STATUS
{
    auto key=new fixture_key_t;
    fixture_path(key,name);
    key->signing=std::wcscmp(algorithm,BCRYPT_ECDSA_P256_ALGORITHM)==0;
    auto ctx=EVP_PKEY_CTX_new_from_name(nullptr,key->signing?"EC":"RSA",nullptr);
    bool ok=ctx && EVP_PKEY_keygen_init(ctx)>0;
    if (ok && key->signing) ok=EVP_PKEY_CTX_set_group_name(ctx,"prime256v1")>0;
    if (ok && !key->signing) ok=EVP_PKEY_CTX_set_rsa_keygen_bits(ctx,2048)>0;
    if (ok) ok=EVP_PKEY_generate(ctx,&key->key)>0;
    EVP_PKEY_CTX_free(ctx);
    if (!ok) { delete key; return NTE_FAIL; }
    *out=reinterpret_cast<NCRYPT_KEY_HANDLE>(key); return 0;
}
static auto fixture_property(NCRYPT_HANDLE,LPCWSTR property,PBYTE data,DWORD size,DWORD) -> SECURITY_STATUS
{
    // The production path must explicitly forbid exporting private keys.
    if (std::wcscmp(property,NCRYPT_EXPORT_POLICY_PROPERTY)==0)
        return size==sizeof(DWORD) && *reinterpret_cast<DWORD*>(data)==0 ? 0 : NTE_FAIL;
    return 0;
}
static auto fixture_finalize(NCRYPT_KEY_HANDLE handle,DWORD) -> SECURITY_STATUS
{
    auto key=reinterpret_cast<fixture_key_t*>(handle);
    auto file=std::fopen(key->path,"wb");
    if (!file) return NTE_FAIL;
    const auto ok=PEM_write_PrivateKey(file,key->key,nullptr,nullptr,0,nullptr,nullptr);
    std::fclose(file); return ok?0:NTE_FAIL;
}
static auto fixture_free(NCRYPT_HANDLE handle) -> SECURITY_STATUS
{
    if (handle && handle!=1)
    { auto key=reinterpret_cast<fixture_key_t*>(handle); EVP_PKEY_free(key->key); delete key; }
    return 0;
}
static auto fixture_delete(NCRYPT_KEY_HANDLE handle,DWORD) -> SECURITY_STATUS
{
    auto key=reinterpret_cast<fixture_key_t*>(handle);
    std::remove(key->path); return fixture_free(handle);
}
static auto fixture_export(NCRYPT_KEY_HANDLE handle,NCRYPT_KEY_HANDLE,LPCWSTR kind,
                          NCryptBufferDesc*,PBYTE output,DWORD capacity,DWORD* size,DWORD) -> SECURITY_STATUS
{
    if (std::wcscmp(kind,BCRYPT_ECCPUBLIC_BLOB)!=0 || capacity<72) return NTE_NOT_SUPPORTED;
    unsigned char point[65]; std::size_t count{};
    auto key=reinterpret_cast<fixture_key_t*>(handle);
    if (!EVP_PKEY_get_octet_string_param(key->key,OSSL_PKEY_PARAM_PUB_KEY,point,sizeof(point),&count) || count!=65)
        return NTE_FAIL;
    const BCRYPT_ECCKEY_BLOB header{BCRYPT_ECDSA_PUBLIC_P256_MAGIC,32};
    std::memcpy(output,&header,sizeof(header));std::memcpy(output+sizeof(header),point+1,64);*size=72;return 0;
}
static auto fixture_sign(NCRYPT_KEY_HANDLE handle,void*,PBYTE hash,DWORD hash_size,
                        PBYTE output,DWORD capacity,DWORD* size,DWORD) -> SECURITY_STATUS
{
    auto key=reinterpret_cast<fixture_key_t*>(handle);
    auto ctx=EVP_PKEY_CTX_new(key->key,nullptr);
    unsigned char der[128];std::size_t count=sizeof(der);
    bool ok=capacity>=64 && ctx && EVP_PKEY_sign_init(ctx)>0 &&
        EVP_PKEY_CTX_set_signature_md(ctx,EVP_sha256())>0 && EVP_PKEY_sign(ctx,der,&count,hash,hash_size)>0;
    EVP_PKEY_CTX_free(ctx);
    if (!ok) return NTE_FAIL;
    const unsigned char* ptr=der;
    auto sig=d2i_ECDSA_SIG(nullptr,&ptr,count);
    if (!sig) return NTE_FAIL;
    ok=BN_bn2binpad(ECDSA_SIG_get0_r(sig),output,32)==32 && BN_bn2binpad(ECDSA_SIG_get0_s(sig),output+32,32)==32;
    ECDSA_SIG_free(sig);*size=64;return ok?0:NTE_FAIL;
}
static auto fixture_rsa(NCRYPT_KEY_HANDLE handle,PBYTE input,DWORD length,void*,PBYTE output,
                       DWORD capacity,DWORD* size,DWORD flags,bool decrypt) -> SECURITY_STATUS
{
    auto key=reinterpret_cast<fixture_key_t*>(handle);
    auto ctx=EVP_PKEY_CTX_new(key->key,nullptr);
    std::size_t count=capacity;
    bool ok=flags==NCRYPT_PAD_OAEP_FLAG && ctx &&
        (decrypt?EVP_PKEY_decrypt_init(ctx):EVP_PKEY_encrypt_init(ctx))>0 &&
        EVP_PKEY_CTX_set_rsa_padding(ctx,RSA_PKCS1_OAEP_PADDING)>0 &&
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx,EVP_sha256())>0;
    if (ok) ok=(decrypt?EVP_PKEY_decrypt(ctx,output,&count,input,length):EVP_PKEY_encrypt(ctx,output,&count,input,length))>0;
    EVP_PKEY_CTX_free(ctx);*size=static_cast<DWORD>(count);return ok?0:NTE_FAIL;
}
static auto fixture_encrypt(NCRYPT_KEY_HANDLE k,PBYTE i,DWORD n,void* p,PBYTE o,DWORD c,DWORD* s,DWORD f)->SECURITY_STATUS
{ return fixture_rsa(k,i,n,p,o,c,s,f,false); }
static auto fixture_decrypt(NCRYPT_KEY_HANDLE k,PBYTE i,DWORD n,void* p,PBYTE o,DWORD c,DWORD* s,DWORD f)->SECURITY_STATUS
{ return fixture_rsa(k,i,n,p,o,c,s,f,true); }
static auto fixture_browser(HWND,LPCWSTR,LPCWSTR url,LPCWSTR,LPCWSTR,INT) -> HINSTANCE
{
    auto file=std::fopen("browser-url.tmp","wb");
    if (!file) return nullptr;
    for (;*url;++url) std::fputc(static_cast<char>(*url),file);
    std::fclose(file);std::remove("browser-url");std::rename("browser-url.tmp","browser-url");
    return reinterpret_cast<HINSTANCE>(33);
}
#define NCryptOpenStorageProvider fixture_provider
#define NCryptOpenKey fixture_open
#define NCryptCreatePersistedKey fixture_create
#define NCryptSetProperty fixture_property
#define NCryptFinalizeKey fixture_finalize
#define NCryptFreeObject fixture_free
#define NCryptDeleteKey fixture_delete
#define NCryptExportKey fixture_export
#define NCryptSignHash fixture_sign
#define NCryptEncrypt fixture_encrypt
#define NCryptDecrypt fixture_decrypt
#define ShellExecuteW fixture_browser
