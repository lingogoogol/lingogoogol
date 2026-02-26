#ifndef LGO_ENCODE_CRYPTO_ECDH_CURVE25519_DEF
#define LGO_ENCODE_CRYPTO_ECDH_CURVE25519_DEF

#include "decl.hpp"

LGO_EXPORT namespace lgo {
    class curve25519: public lgo {
    public:
        using type = numzs8<intpc::val_v<U"0">, intpc::val_v<U"20">>;

        curve25519();

        auto private_key() const -> const type&;
        auto public_key() const -> const type&;
        auto shared() const -> const type&;

        auto make_shared(const type& public_key) -> void;
    private:
		constexpr static intpc bits{ U"FF"_intpc };
		inline static const type a24{ U"1DB41"_intpc };
        inline static const type p{ (type{ U"1"_intpc } << U"FF"_intpc) - type{ U"13"_intpc } };

        static auto x25519(const type& k, const type& u, type& out) -> void;
        static auto cswap(const type& swap, type& x_2, type& x_3) -> void;

        type m_private_key{};
        type m_public_key{};
        type m_shared{};
    };
}

#endif