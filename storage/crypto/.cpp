module storage.crypto;

import std;

import external.OpenSSL;

auto sha256withrsa(std::string in, std::string key) -> std::string
{
    EVP_PKEY* private_key{};
    OSSL_DECODER_CTX* ossl_decoder
    {
        OSSL_DECODER_CTX_new_for_pkey
        (
            &private_key,
            "PEM",
            nullptr,
            "RSA",
            EVP_PKEY_KEYPAIR,
            nullptr,
            nullptr
        )
    };
    auto pem_data{ reinterpret_cast<const unsigned char*>(key.data()) };
    auto pem_size{ key.size() };
    OSSL_DECODER_from_data(ossl_decoder, &pem_data, &pem_size);
    EVP_PKEY_CTX* openssl_pkey{ EVP_PKEY_CTX_new(private_key, nullptr) };
    EVP_SIGNATURE* sign_algo{ EVP_SIGNATURE_fetch(nullptr, "RSA-SHA256", nullptr) };
    EVP_PKEY_sign_message_init(openssl_pkey, sign_algo, nullptr);
    EVP_PKEY_sign_message_update(openssl_pkey, reinterpret_cast<const unsigned char*>(in.data()), in.size());
    std::size_t out_size{};
    EVP_PKEY_sign_message_final(openssl_pkey, nullptr, &out_size);
    std::string out{};
    out.resize(out_size);
    EVP_PKEY_sign_message_final(openssl_pkey, reinterpret_cast<unsigned char*>(out.data()), &out_size);
    return out;
}
