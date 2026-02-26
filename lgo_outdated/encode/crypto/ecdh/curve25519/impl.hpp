#ifndef LGO_ENCODE_CRYPTO_ECDH_CURVE25519_IMPL
#define LGO_ENCODE_CRYPTO_ECDH_CURVE25519_IMPL

#include "def.hpp"

namespace lgo {
	curve25519::curve25519(): m_private_key{ rand<type>() } {
		m_private_key[U"0"_intpc] &= U"F8"_intp8;
		m_private_key[U"1F"_intpc] &= U"7F"_intp8;
		m_private_key[U"1F"_intpc] |= U"40"_intp8;
		x25519(m_private_key, type{ U"9"_intpc }, m_public_key);
	}

	auto curve25519::private_key() const -> const type& {
		return m_private_key;
	}

	auto curve25519::public_key() const -> const type& {
		return m_public_key;
	}

	auto curve25519::shared() const -> const type& {
		return m_shared;
	}

	auto curve25519::make_shared(const type& public_key) -> void {
		x25519(m_private_key, public_key, m_shared);
		return;
	}

	auto curve25519::x25519(const type& k, const type& u, type& out) -> void {
		type x_1{ u }, x_2{ U"1"_intpc }, z_2{ U"0"_intpc }, x_3{ u }, z_3{ U"1"_intpc }, swap{ U"0"_intpc };

		for (intpc t{ bits - U"1"_intpc }; (t < intpc::max()).basic(); --t) {
			std::cout << t.basic() << '\n';
			type k_t{ (k >> t) & type{ U"1"_intpc } };
			swap ^= k_t;
			cswap(swap, x_2, x_3);
			cswap(swap, z_2, z_3);
			swap = k_t;

			type A{ (x_2 + z_2).moddw(p) };
			type AA{ A.powmoddw(type{ U"2"_intpc }, p) };
			type B{ (x_2 - z_2).moddw(p) };
			type BB{ B.powmoddw(type{ U"2"_intpc }, p) };
			type E{ (AA - BB).moddw(p) };
			type C{ (x_3 + z_3).moddw(p) };
			type D{ (x_3 - z_3).moddw(p) };
			type DA{ (D * A).moddw(p) };
			type CB{ (C * B).moddw(p) };
			x_3 = (DA + CB).moddw(p).powmoddw(type{ U"2"_intpc }, p);
			z_3 = (x_1 * (DA - CB).moddw(p).powmoddw(type{ U"2"_intpc }, p)).moddw(p);
			x_2 = (AA * BB).moddw(p);
			z_2 = (E * (AA + (a24 * E).moddw(p)).moddw(p)).moddw(p);
		}

		cswap(swap, x_2, x_3);
		cswap(swap, z_2, z_3);
		out = (x_2 * z_2.powmoddw(p - type{ U"2"_intpc }, p)).moddw(p);
		return;
	}

	auto curve25519::cswap(const type& swap, type& x_2, type& x_3) -> void {
		type dummy{ (type{ U"0"_intpc }.sub_overflow(swap)).moddw(p) & (x_2 ^ x_3) };
		x_2 = x_2 ^ dummy;
		x_3 = x_3 ^ dummy;
		return;
	}
}

#endif