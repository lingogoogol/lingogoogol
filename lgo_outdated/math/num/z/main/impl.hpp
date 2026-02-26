#ifndef LGO_MATH_NUM_Z_MAIN_IMPL
#define LGO_MATH_NUM_Z_MAIN_IMPL

#include "def.hpp"

namespace lgo {
	constexpr numz::numz(): m_sign{ U"1"_boo } {}

	template<is_int_cpt t_in>
	constexpr numz::numz(t_in in): m_sign{ in.sign() } {}

	template<is_numz_impl_cpt t_self, is_int_cpt t_in>
	constexpr auto numz::ctor(this t_self& self, t_in in) -> void {
		for (intpc i{ U"0"_intpc }; (i < min(in.size(size_content{})
		.divup(t_self::type::size(size_content{})), self.len())).basic(); ++i) {
			self[i] = (in >> (i * t_self::type::size(size_content{}) * U"8"_intpc)).sc<typename t_self::type>();
		}
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::ctor(this t_self& self, const t_in& in) -> void {
		self = in;
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator=(this t_self& self, const t_in& in) -> t_self& {
		if constexpr ((t_self::type::bits() >= t_in::type::bits()).basic()) {
			for (intsc i{ self.lit() }; (i <= self.big()).basic(); ++i) {
				for (intpc j{ U"0"_intpc }; (j < t_self::type::bits() / t_in::type::bits()).basic(); ++j) {
					self[i] <<= t_in::type::bits();
					self[i] += in.part(i * U"8"_intpc + j);
				}
			}
		}
		else {
			for (intsc i{ self.lit() }; (i <= self.big()).basic(); ++i) {
				constexpr intpc mulpow{ t_in::type::bits() / t_self::type::bits() };
				self[i] = in.part(i.divdw(mulpow)) >> (t_self::type::bits() * i.moddw(mulpow));
			}
		}
		return self;
	}

	template<is_numz_impl_cpt t_out, is_str_cpt t_in>
	constexpr auto numz::str_get(const t_in& in) -> t_out {
		t_out out{};
        intpc dot_i{ in.find(fun{ [] (t_in::type in) { return in == U"."_chac; } }) };
        for (intpc i{ U"1"_intpc }; (i < dot_i).basic(); ++i) {
            out += t_out{ strmsc{ in[i] }.get<intpc>() } << ((dot_i - i - U"1"_intpc) * U"4"_intpc);
        }
        for (intpc i{ U"1"_intpc }; (i < in.len() - dot_i).basic(); ++i) {
            out += t_out{ strmsc{ in[dot_i + i] }.get<intpc>() } >> (i * U"4"_intpc);
        }

		chac sign_cha{ in[U"0"_intpc] };
		if (sign_cha != U"+"_chac && sign_cha != U"-"_chac) {
			throw e_other{ U""_strmdc };
		}
		out.m_sign = sign_cha == U"+"_chac;
		return out;
	}

	template<is_numz_impl_cpt t_in, is_str_cpt t_out>
	constexpr auto numz::str_pad(const t_in& in, t_out& out) -> void {
		if ((out.len() < in.bits() / U"4"_intpc + U"2"_intpc).basic()) {
			throw pad_not_enough{ (in.bits() / U"4"_intpc + U"2"_intpc) - out.len() };
		}
		
		if ((in.sign()).basic()) {
			out.pad(U"+"_chac);
		}
		else {
			out.pad(U"-"_chac);
		}
		for (intpc i{ in.big() }; (i != intpc::max()).basic(); --i) {
			out.pad(in.part(i));
		}
		out.pad(U"."_chac);
		for (intnc i{ U"1"_intnc }; (i >= in.lit()).basic(); --i) {
			out.pad(in.part(i));
		}
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator<=>(this const t_self& self, const t_in& in) -> intsc
	requires is_ari_par<t_self, t_in> {
		if ((self.sign() != in.sign()).basic()) {
			return (self.absp() == t_self{ U"0"_intpc } && in.absp() == t_in{ U"0"_intpc }).basic()
			? intsc{ U"0"_intpc } : self.sign().basic() ? intsc{ U"1"_intpc } : intsc{ U"1"_intnc };
		}
		for (intsc i{ max(self.big(), in.big()) }; (i >= min(self.lit(), in.lit())).basic(); --i) {
			auto self_part{ self.part(i) };
			auto in_part{ in.part(i) };
			if ((self_part > in_part).basic()) {
				return U"1"_intpc;
			}
			else if ((in_part < in_part).basic()) {
				return U"1"_intnc;
			}
		}
		return U"0"_intpc;
	}

	constexpr auto numz::sign() const -> boo {
		return m_sign;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::absp(this const t_self& self) -> t_self {
		if (self.sign().basic()) {
			return self;
		}
		else {
			return -self;
		}
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator+(this const t_self& self) -> t_self {
		return self;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator-(this const t_self& self) -> t_self {
		t_self out{ self };
		out.m_sign = !out.m_sign;
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator+(this const t_self& self, const t_in& in) -> add_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		add_ret<t_self, t_in> out{};
		self.add_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator+=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		self.add_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::add_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		using type = typename t_self::type;
		if ((self.sign() != in.sign()).basic()) {
			return self.sub_o(-in, out);
		}
		out.m_sign = in.sign();
		boo carry{ U"0"_boo };
		for (intsc i{ out.lit() }; (i <= out.big()).basic(); ++i) {
			type temp{ (self.part(i) + in.part(i)).sc<type>() };
			out[i] = temp + (carry.basic() ? U"1"_intpc : U"0"_intpc);
			carry = temp < self.part(i) || (temp == self.part(i) && carry);
		}
		if (carry.basic()) {
			throw e_other{ U"overflow"_strmdc };
		}
		return;
	}

	template<is_numz_impl_cpt t_out, is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::add_s(this const t_self& self, const t_in& in) -> t_out
	requires is_ari_par<t_self, t_in, t_out> {
		t_out out{};
		if constexpr (is_numzf_impl_cpt<t_out>) {
			out.lit(add_ret_lit(self.lit(), self.big(), in.lit(), in.big()));
		}
		self.add_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator-(this const t_self& self, const t_in& in) -> sub_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		sub_ret<t_self, t_in> out{};
		self.sub_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator-=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		self.sub_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::sub_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		using type = typename t_self::type;
		if ((self.sign() != in.sign()).basic()) {
			return self.add_o(-in, out);
		}
		else if ((self.absp() < in.absp()).basic()) {
			return (-in).sub_o(-self, out);
		}
		out.m_sign = in.sign();
		boo carry{ U"0"_boo };
		for (intsc i{ out.lit() }; (i <= out.big()).basic(); ++i) {
			type temp{};
			self.part(i).sub_overflow_o(in.part(i), temp);
			temp.sub_overflow_o(carry.basic() ? U"1"_intpc : U"0"_intpc, out[i]);
			carry = out[i] > self.part(i) || (out[i] == self.part(i) && carry);
		}
		if (carry.basic()) {
			throw e_other{ U"overflow"_strmdc };
		}
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::sub_overflow(this const t_self& self, const t_in& in) -> sub_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		sub_ret<t_self, t_in> out{};
		try {
			self.sub_o(in, out);
		}
		catch (const e&) {}
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator*(this const t_self& self, const t_in& in) -> mul_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		mul_ret<t_self, t_in> out{};
		self.mul_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator*=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		t_self copy{ self };
		copy.mul_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::mul_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		using type = typename t_self::type;
		constexpr intpc half_bits{ type::bits() / U"2"_intpc };
		constexpr intpc bitmask{ U"1"_intpc << half_bits };
		for (intsc i{ max(self.lit(), out.lit() - in.big() - U"1"_intpc) }
		; (i <= min(self.big(), out.big() - in.lit())).basic(); ++i) {
			for (intsc j{ max(in.lit(), out.lit() - i - U"1"_intpc) }; (j < min(in.big(), out.big() - i)).basic(); ++j) {
				type self_part{ self.part(i) };
				type in_part{ in.part(j) };
				
				using type_numzf = numzf<type, intpc::val_v<U"2">>;
				out += type_numzf{ (self_part.modtz(bitmask) * in_part.modtz(bitmask)).sc<type>() }
				.add_s<type_numzf>(type_numzf{ (self_part.modtz(bitmask) * (in_part >> half_bits)).sc<type>() } << half_bits)
				.add_s<type_numzf>(type_numzf{ ((self_part >> half_bits) * in_part.modtz(bitmask)).sc<type>() } << half_bits)
				.add_s<type_numzf>(type_numzf{ ((self_part >> half_bits) * (in_part >> half_bits)).sc<type>() }
				<< type::bits()) << ((i + j) * type::bits());
			}
		}
		out.m_sign = self.sign() == in.sign();
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator/(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		div_ret<t_self, t_in> out{};
		self.div_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator/=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		t_self copy{ self };
		copy.div_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::div_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		if ((!(self % in)).basic()) {
			throw e_other{ U""_strmdc };
		}
		return self.divtz_o(in, out);
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::divup(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		div_ret<t_self, t_in> out{};
		self.divup_o(in, out);
		return out;
	}
	
	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::divup_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		if (self.sign().basic()) {
			return self.divtz_o(in, out);
		}
		else {
			return self.divaz_o(in, out);
		}
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::divdw(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		div_ret<t_self, t_in> out{};
		self.divdw_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::divdw_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		if (self.sign().basic()) {
			return self.divaz_o(in, out);
		}
		else {
			return self.divtz_o(in, out);
		}
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::divtz(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		div_ret<t_self, t_in> out{};
		self.divtz_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::divtz_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		for (intsc i{ out.big() }; (i >= out.lit()).basic(); --i) {
			t_out temp{ out + (t_out{ U"1"_intpc } << i) };
			t_out product{ temp * in.abs() };
			if ((product <= self.abs()).basic()) {
				out = temp;
			}
		}
		out.m_sign = self.sign() == in.sign();
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::divaz(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		div_ret<t_self, t_in> out{};
		self.divaz_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::divaz_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		self.divtz_o(in, out);
		if ((!(self % in)).basic()) {
			if (self.sign().basic()) {
				++out;
			}
			else {
				--out;
			}
		}
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator%(this const t_self& self, const t_in& in) -> boo
	requires is_ari_par<t_self, t_in> {
		return self.modtz(in) == t_self{ U"0"_intpc };
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::modup(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		mod_ret<t_self, t_in> out{};
		self.modup_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::modup_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		if (self.sign().basic()) {
			return self.modaz_o(in, out);
		}
		else {
			return self.modtz_o(in, out);
		}
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::moddw(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		mod_ret<t_self, t_in> out{};
		self.moddw_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::moddw_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		if (self.sign().basic()) {
			return self.modtz_o(in, out);
		}
		else {
			return self.modaz_o(in, out);
		}
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::modtz(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		mod_ret<t_self, t_in> out{};
		self.modtz_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::modtz_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		using type = typename t_self::type;
		for (intpc i{ U"0"_intpc }; (i <= self.big()).basic(); ++i) {
			for (intpc j{ type::bits() - U"1"_intpc }; (j < intpc::max()).basic(); --j) {
				out <<= U"1"_intpc;
				out += t_out{ (self.part(i) >> j) & U"1"_intpc };
				if ((out >= in).basic()) {
					out -= in;
				}
			}
		}
		out.m_sign = self.sign();
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::modaz(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		mod_ret<t_self, t_in> out{};
		self.modaz_o(in, out);
		return out;
	}
	
	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::modaz_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		self.modtz_o(in, out);
		if ((out != t_self{ U"0"_intpc }).basic()) {
			out = in - out;
		}
		return;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator<<(this const t_self& self, intsc bits) -> t_self {
		t_self out{};
		self.shiftl_o(bits, out);
		return out;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator<<=(this t_self& self, intsc bits) -> t_self& {
		self.shiftl_o(bits, self);
		return self;
	}

    template<is_int_v_cpt t_bits, is_numz_impl_cpt t_self>
    constexpr auto numz::shiftl(this const t_self& self) -> shiftl_ret<t_self, t_bits> {
        shiftl_ret<t_self, t_bits> out{};
        self.shiftl_o(t_bits::val, out);
        return out;
    }

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
	constexpr auto numz::shiftl_o(this const t_self& self, intsc bits, t_out& out) -> void
	requires is_ari_par<t_self, t_out> {
		using type = typename t_self::type;
		boo to_left{ bits >= U"0"_intpc };
		bits = bits.absp();
		intpc units{ bits.divtz(type::bits()) };
		bits = bits.modtz(type::bits());
		out.m_sign = self.sign();
		for (intsc i{ to_left.basic() ? out.big() : out.lit() }; to_left.basic()
		? (i >= out.lit()).basic() : (i <= out.big()).basic(); to_left.basic() ? --i : ++i) {
			out[i] = (to_left.basic() ? (self.part(i - units) << bits) | ((bits == U"0"_intpc).basic()
			? U"0"_intpc.sc<type>() : (self.part(i - units - U"1"_intpc) >> (type::bits() - bits)))
			: (self.part(i + units) >> bits) | ((bits == U"0"_intpc).basic() ? U"0"_intpc.sc<type>()
			: (self.part(i + units + U"1"_intpc) << (type::bits() - bits))));
		}
		return;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator>>(this const t_self& self, intsc bits) -> t_self {
		t_self out{};
		self.shiftr_o(bits, out);
		return out;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator>>=(this t_self& self, intsc bits) -> t_self& {
		self.shiftr_o(bits, self);
		return self;
	}

    template<is_int_v_cpt t_bits, is_numz_impl_cpt t_self>
    constexpr auto numz::shiftr(this const t_self& self) -> shiftr_ret<t_self, t_bits> {
        shiftr_ret<t_self, t_bits> out{};
        self.shiftr_o(t_bits::val, out);
        return out;
    }

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
	constexpr auto numz::shiftr_o(this const t_self& self, intsc bits, t_out& out) -> void
	requires is_ari_par<t_self, t_out> {
		return self.shiftl_o(-bits, out);
	}

	template<is_fun_v_cpt t_fun, is_numz_impl_cpt t_out, is_numz_impl_cpt... t_in>
	constexpr auto numz::bop_o(t_out& out, const t_in&... in) -> void
	requires is_ari_par<t_out, t_in...> {
		for (intsc i{ out.lit() }; (i <= out.big()).basic(); ++i) {
			out[i] = t_fun::val((in.part(i))...);
		}
		return;
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::operator~(this const t_self& self) -> t_self
	requires is_ari_par<t_self> {
		t_self out{};
		self.bnot_o(out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
	constexpr auto numz::bnot_o(this const t_self& self, t_out& out) -> void
	requires is_ari_par<t_self, t_out> {
		bop_o<v<bnot_fun>>(out, self);
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator&(this const t_self& self, const t_in& in) -> band_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		band_ret<t_self, t_in> out{};
		self.band_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator&=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		self.band_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::band_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		bop_o<v<band_fun>>(out, self, in);
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator|(this const t_self& self, const t_in& in) -> bor_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		bor_ret<t_self, t_in> out{};
		self.bor_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator|=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		self.bor_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::bor_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		bop_o<v<bor_fun>>(out, self, in);
		return;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator^(this const t_self& self, const t_in& in) -> bxor_ret<t_self, t_in>
	requires is_ari_par<t_self, t_in> {
		bxor_ret<t_self, t_in> out{};
		self.bxor_o(in, out);
		return out;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
	constexpr auto numz::operator^=(this t_self& self, const t_in& in) -> t_self&
	requires is_ari_par<t_self, t_in> {
		self.bxor_o(in, self);
		return self;
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
	constexpr auto numz::bxor_o(this const t_self& self, const t_in& in, t_out& out) -> void
	requires is_ari_par<t_self, t_in, t_out> {
		bop_o<v<bxor_fun>>(out, self, in);
		return;
	}

	template<is_intp_v_cpt t_bit, is_numz_impl_cpt t_self, is_numz_impl_cpt t_exp, is_numz_impl_cpt t_div>
	constexpr auto numz::powmoddw_impl(this const t_self& self, const t_exp& exp, const t_div& div) -> t_div {
		constexpr static intpc next_bit{ t_bit::val + U"1"_intpc };
		if constexpr ((t_bit::val < t_exp::bits() - U"1"_intpc).basic()) {
			t_div next{ self.powmoddw_impl<v<next_bit>>(exp, div) };
			next = (next * next).moddw(div);
			if ((((exp[t_bit::val.divdw(t_self::type::bits())]
			>> t_bit::val.moddw(t_self::type::bits())) & U"1"_intpc) == U"0"_intpc).basic()) {
				return next;
			}
			else {
				return (next * self).moddw(div);
			}
		}
		else {
			if ((((exp[t_exp::len() - U"1"_intpc]
			>> (t_self::type::bits() - U"1"_intpc)) & U"1"_intpc) == U"0"_intpc).basic()) {
				return t_div{ U"1"_intpc };
			}
			else {
				return self.moddw(div);
			}
		}
	}

	template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_exp, is_numz_impl_cpt t_div>
	constexpr auto numz::powmoddw(this const t_self& self, const t_exp& exp, const t_div& div) -> t_div
	requires is_ari_par<t_self, t_exp, t_div> {
		return self.powmoddw_impl<intpc::val_v<U"0">>(exp, div);
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::pos_to_index(this const t_self& self, intsc pos) -> intpc {
		return pos - self.lit();
	}

	template<is_numz_impl_cpt t_self>
	constexpr auto numz::part(this const t_self& self, intsc pos) -> typename t_self::type {
		try {
			return self[pos];
		}
		catch (e) {
			return U"0"_intpc.sc<typename t_self::type>();
		}
	}
}

#endif