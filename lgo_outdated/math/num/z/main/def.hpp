#ifndef LGO_MATH_NUM_Z_MAIN_DEF
#define LGO_MATH_NUM_Z_MAIN_DEF

#include "cpt.hpp"

#include "../s/base_decl.hpp"
#include "../s/cvt.hpp"
#include "../f/base_decl.hpp"
#include "../d/decl.hpp"

namespace lgo {
	template<typename... t_in>
	concept is_ari_par = (is_numz_impl_cpt<rm_qcvr<t_in>> && ...) && is_same_cpt<typename t_in::type...>;
	
	class numz {
	protected:
		constexpr numz();
		template<is_int_cpt t_in>
		constexpr numz(t_in in);
		constexpr numz(const numz& in) = default;

		template<is_numz_impl_cpt t_self, is_int_cpt t_in>
		constexpr auto ctor(this t_self& self, t_in in) -> void;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto ctor(this t_self& self, const t_in& in) -> void;
	public:
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator=(this t_self& self, const t_in& in) -> t_self&;

		template<is_numz_impl_cpt t_out, is_str_cpt t_in>
		constexpr static auto str_get(const t_in& in) -> t_out;
		template<is_numz_impl_cpt t_in, is_str_cpt t_out>
		constexpr static auto str_pad(const t_in& in, t_out& out) -> void;
	public:
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator<=>(this const t_self& self, const t_in& in)
		-> intsc requires is_ari_par<t_self, t_in>;
	protected:
		constexpr static fun add_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return min(self_lit, in_lit); }
		};
		constexpr static fun add_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return max(self_big, in_big); }
		};
		constexpr static fun sub_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return min(self_lit, in_lit); }
		};
		constexpr static fun sub_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return max(self_big, in_big); }
		};
		constexpr static fun mul_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return self_lit + in_lit; }
		};
		constexpr static fun mul_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return self_big + in_big; }
		};
		constexpr static fun div_ret_lit{
			[] (intsc self_lit, intsc, intsc, intsc in_big) constexpr -> intsc { return self_lit - in_big; }
		};
		constexpr static fun div_ret_big{
			[] (intsc, intsc self_big, intsc in_lit, intsc) constexpr -> intsc { return self_big - in_lit; }
		};
		constexpr static fun mod_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return min(self_lit, in_lit); }
		};
		constexpr static fun mod_ret_big{
			[] (intsc, intsc, intsc, intsc in_big) constexpr -> intsc { return in_big; }
		};
		constexpr static fun band_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return max(self_lit, in_lit); }
		};
		constexpr static fun band_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return min(self_big, in_big); }
		};
		constexpr static fun bor_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return min(self_lit, in_lit); }
		};
		constexpr static fun bor_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return max(self_big, in_big); }
		};
		constexpr static fun bxor_ret_lit{
			[] (intsc self_lit, intsc, intsc in_lit, intsc) constexpr -> intsc { return min(self_lit, in_lit); }
		};
		constexpr static fun bxor_ret_big{
			[] (intsc, intsc self_big, intsc, intsc in_big) constexpr -> intsc { return max(self_big, in_big); }
		};
		constexpr static fun shiftl_ret_lit{
			[] (intsc self_lit, intpc type_size, intsc bits) constexpr -> intsc { return self_lit + bits.divdw(type_size); }
		};
		constexpr static fun shiftl_ret_big{
			[] (intsc self_big, intpc type_size, intsc bits) constexpr -> intsc { return self_big + bits.divup(type_size); }
		};
		constexpr static fun shiftr_ret_lit{
			[] (intsc self_lit, intpc type_size, intsc bits) constexpr -> intsc { return self_lit - bits.divup(type_size); }
		};
		constexpr static fun shiftr_ret_big{
			[] (intsc self_big, intpc type_size, intsc bits) constexpr -> intsc { return self_big - bits.divdw(type_size); }
		};
	private:
		template<is_numz_cpt t_self, is_numz_cpt t_in, is_fun_v_cpt t_lit, is_fun_v_cpt t_big>
		struct ari_ret_stu {
			constexpr static boo is_both_numzs{ is_numzs_boo<t_self> && is_numzs_boo<t_in> };
			constexpr static intsc lit{ t_lit::val(t_self::lit(), t_self::big(), t_in::lit(), t_in::big()) };
			constexpr static intpc len{ t_big::val
			(t_self::lit(), t_self::big(), t_in::lit(), t_in::big()) - lit + U"1"_intpc };
			using type = tcond<arrmc<v<is_both_numzs>, numzs_to_interface<numzs<typename t_self::type, v<lit>, v<len>>>>
			, arrmc<boo::val_v<U"t">, numzd<typename t_self::type>>>;
		};
		template<is_numz_cpt t_self, is_numz_cpt t_in, is_fun_v_cpt t_lit, is_fun_v_cpt t_big>
		requires is_ari_par<t_self, t_in>
		using ari_ret = ari_ret_stu<t_self, t_in, t_lit, t_big>::type;

		template<is_numz_cpt t_self, is_int_v_cpt t_bits, is_fun_v_cpt t_lit, is_fun_v_cpt t_big>
		struct shift_ret_stu {
			constexpr static intsc lit{ t_lit::val(t_self::lit(), t_self::type::size(size_content{}), t_bits::val) };
			constexpr static intpc len{ t_big::val(t_self::big()
			, t_self::type::size(size_content{}), t_bits::val) - lit + U"1"_intpc };
			using type = tcond<arrmc<is_numzs_v<t_self>, numzs_to_interface<numzs<typename t_self::type, v<lit>, v<len>>>>
			, arrmc<boo::val_v<U"t">, numzd<typename t_self::type>>>;
		};
		template<is_numz_cpt t_self, is_int_v_cpt t_bits, is_fun_v_cpt t_lit, is_fun_v_cpt t_big>
		using shift_ret = shift_ret_stu<t_self, t_bits, t_lit, t_big>::type;
	protected:
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using add_ret = ari_ret<t_self, t_in, v<add_ret_lit>, v<add_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using sub_ret = ari_ret<t_self, t_in, v<sub_ret_lit>, v<sub_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using mul_ret = ari_ret<t_self, t_in, v<mul_ret_lit>, v<mul_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using div_ret = ari_ret<t_self, t_in, v<div_ret_lit>, v<div_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using mod_ret = ari_ret<t_self, t_in, v<mod_ret_lit>, v<mod_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using band_ret = ari_ret<t_self, t_in, v<band_ret_lit>, v<band_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using bor_ret = ari_ret<t_self, t_in, v<bor_ret_lit>, v<bor_ret_big>>;
		template<is_numz_cpt t_self, is_numz_cpt t_in>
		using bxor_ret = ari_ret<t_self, t_in, v<bxor_ret_lit>, v<bxor_ret_big>>;
		template<is_numz_cpt t_self, is_int_v_cpt t_bits>
		using shiftl_ret = shift_ret<t_self, t_bits, v<shiftl_ret_lit>, v<shiftl_ret_big>>;
		template<is_numz_cpt t_self, is_int_v_cpt t_bits>
		using shiftr_ret = shift_ret<t_self, t_bits, v<shiftr_ret_lit>, v<shiftr_ret_big>>;
	public:
		constexpr auto sign() const -> boo;
		template<is_numz_impl_cpt t_self>
		constexpr auto absp(this const t_self& self) -> t_self;
		template<is_numz_impl_cpt t_self>
		constexpr auto operator+(this const t_self& self) -> t_self;
		template<is_numz_impl_cpt t_self>
		constexpr auto operator-(this const t_self& self) -> t_self;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator+(this const t_self& self, const t_in& in) -> add_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator+=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto add_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;
		template<is_numz_impl_cpt t_out, is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto add_s(this const t_self& self, const t_in& in) -> t_out
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator-(this const t_self& self, const t_in& in) -> sub_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator-=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto sub_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto sub_overflow(this const t_self& self, const t_in& in) -> sub_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator*(this const t_self& self, const t_in& in) -> mul_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator*=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto mul_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator/(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator/=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto div_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto divup(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto divup_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto divdw(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto divdw_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto divtz(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto divtz_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto divaz(this const t_self& self, const t_in& in) -> div_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto divaz_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator%(this const t_self& self, const t_in& in) -> boo
		requires is_ari_par<t_self, t_in>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto modup(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto modup_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto moddw(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto moddw_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto modtz(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto modtz_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto modaz(this const t_self& self, const t_in& in) -> mod_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto modaz_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;
		
		template<is_numz_impl_cpt t_self>
		constexpr auto operator<<(this const t_self& self, intsc bits) -> t_self;
		template<is_numz_impl_cpt t_self>
		constexpr auto operator<<=(this t_self& self, intsc bits) -> t_self&;
		template<is_int_v_cpt t_bits, is_numz_impl_cpt t_self>
		constexpr auto shiftl(this const t_self& self) -> shiftl_ret<t_self, t_bits>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
		constexpr auto shiftl_o(this const t_self& self, intsc bits, t_out& out) -> void
		requires is_ari_par<t_self, t_out>;
		
		template<is_numz_impl_cpt t_self>
		constexpr auto operator>>(this const t_self& self, intsc bits) -> t_self;
		template<is_numz_impl_cpt t_self>
		constexpr auto operator>>=(this t_self& self, intsc bits) -> t_self&;
		template<is_int_v_cpt t_bits, is_numz_impl_cpt t_self>
		constexpr auto shiftr(this const t_self& self) -> shiftr_ret<t_self, t_bits>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
		constexpr auto shiftr_o(this const t_self& self, intsc bits, t_out& out) -> void
		requires is_ari_par<t_self, t_out>;
	private:
		template<is_fun_v_cpt t_fun, is_numz_impl_cpt t_out, is_numz_impl_cpt... t_in>
		constexpr static auto bop_o(t_out& out, const t_in&... in) -> void
		requires is_ari_par<t_out, t_in...>;

		constexpr static fun bnot_fun{
			[] <is_intp_cpt t_in> (t_in in) constexpr -> t_in { return ~in; }
		};
		constexpr static fun band_fun{
			[] <is_intp_cpt t_in> (t_in in1, t_in in2) constexpr -> t_in { return in1 & in2; }
		};
		constexpr static fun bor_fun{
			[] <is_intp_cpt t_in> (t_in in1, t_in in2) constexpr -> t_in { return in1 | in2; }
		};
		constexpr static fun bxor_fun{
			[] <is_intp_cpt t_in> (t_in in1, t_in in2) constexpr -> t_in { return in1 ^ in2; }
		};
	public:
		template<is_numz_impl_cpt t_self>
		constexpr auto operator~(this const t_self& self) -> t_self
		requires is_ari_par<t_self>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_out>
		constexpr auto bnot_o(this const t_self& self, t_out& out) -> void
		requires is_ari_par<t_self, t_out>;
		
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator&(this const t_self& self, const t_in& in) -> band_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator&=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto band_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;
		
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator|(this const t_self& self, const t_in& in) -> bor_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator|=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto bor_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;

		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator^(this const t_self& self, const t_in& in) -> bxor_ret<t_self, t_in>
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in>
		constexpr auto operator^=(this t_self& self, const t_in& in) -> t_self&
		requires is_ari_par<t_self, t_in>;
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_in, is_numz_impl_cpt t_out>
		constexpr auto bxor_o(this const t_self& self, const t_in& in, t_out& out) -> void
		requires is_ari_par<t_self, t_in, t_out>;
	private:
		template<is_intp_v_cpt t_bit, is_numz_impl_cpt t_self, is_numz_impl_cpt t_exp, is_numz_impl_cpt t_div>
		constexpr auto powmoddw_impl(this const t_self& self, const t_exp& exp, const t_div& div) -> t_div;
	public:
		template<is_numz_impl_cpt t_self, is_numz_impl_cpt t_exp, is_numz_impl_cpt t_div>
		constexpr auto powmoddw(this const t_self& self, const t_exp& exp, const t_div& div) -> t_div
		requires is_ari_par<t_self, t_exp, t_div>;
	protected:
		template<is_numz_impl_cpt t_self>
		constexpr auto pos_to_index(this const t_self& self, intsc pos) -> intpc;
		template<is_numz_impl_cpt t_self>
		constexpr auto part(this const t_self& self, intsc pos) -> typename t_self::type;
	private:
		boo m_sign{};
	};
}

#endif