module;

#include <openssl/evp.h>
#include <openssl/decoder.h>

export module external.OpenSSL;

constexpr auto EVP_PKEY_KEYPAIR_export{ EVP_PKEY_KEYPAIR };
#undef EVP_PKEY_KEYPAIR

export {
    using ::OSSL_DECODER_CTX;
    using ::EVP_PKEY;
    using ::EVP_PKEY_CTX;
    using ::EVP_SIGNATURE;

    using ::OSSL_DECODER_CTX_new_for_pkey;
    using ::OSSL_DECODER_from_data;
    using ::EVP_PKEY_sign_message_init;
    using ::EVP_PKEY_sign_message_update;
    using ::EVP_PKEY_sign_message_final;
    using ::EVP_PKEY_CTX_new;
    using ::EVP_SIGNATURE_fetch;

    constexpr auto EVP_PKEY_KEYPAIR{ EVP_PKEY_KEYPAIR_export };
}
