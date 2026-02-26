#ifndef LGO_BASIC_TYPE_INT_MAIN_DEF
#define LGO_BASIC_TYPE_INT_MAIN_DEF

#include "cpt.hpp"

#include "opt/base/cpt.hpp"
#include "../p/basic.hpp"
#include "../p/alias.hpp"
#include "../p/cpt.hpp"
#include "../p/decl.hpp"
#include "../n/decl.hpp"
#include "../s/alias.hpp"
#include "../../fun/cast_impl.hpp"
#include "../../fun/common.hpp"
#include "../../boo/decl.hpp"
#include "../../deduce/cond_basic.hpp"
#include "../../../seq/str/md/alias.hpp"
#include "../../../seq/str/main/cpt.hpp"
#include "../../../seq/mem/main/cpt.hpp"
#include "../../../size/stack/.hpp"
#include "../../../size/content/.hpp"
#include "../../../size/mem/decl.hpp"

namespace lgo {
	template<is_basic_intp_cpt t_type>
	class int_class {
	protected:
		using type = t_type;
		
        template<typename t_in>
        constexpr static auto size(t_in) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content>;
	public:
		constexpr int_class();
		constexpr int_class(type in);
		constexpr int_class(const int_class&) = delete;

		constexpr auto operator=(const int_class&) -> int_class& = delete;
		
		constexpr static auto bits() -> intpc;
		constexpr static auto bytes() -> intpc;
		
		template<is_int_cpt u_out, is_str_cpt u_in>
		constexpr static auto str_get(const u_in& in) -> u_out;
		template<is_int_cpt u_out, is_str_cpt u_in, is_int_base_cpt u_base>
		constexpr static auto str_get(const u_in& in, u_base) -> u_out;
		template<is_int_cpt u_in, is_str_cpt u_out>
		constexpr static auto str_pad(u_in in, u_out& out) -> void;
		template<is_int_cpt u_in, is_str_cpt u_out, is_int_base_cpt u_base>
		constexpr static auto str_pad(u_in in, u_out& out, u_base) -> void;
		
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator<=>(this t_self self, t_in in) -> intsc;
	private:
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using add_ret_data = tcond_basic<(sizeof(typename t_in1::int_class::type) >= sizeof(typename t_in2::int_class::type))
		, typename t_in1::int_class::type, typename t_in2::int_class::type>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using mul_ret_data = add_ret_data<t_in1, t_in2>;
	protected:
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using add_ret = tcond_basic<is_intp_cpt<t_in1> && is_intp_cpt<t_in2>
		, intp_class<add_ret_data<t_in1, t_in2>>, tcond_basic<is_intn_cpt<t_in1> && is_intn_cpt<t_in2>
		, intn_class<add_ret_data<t_in1, t_in2>>, ints_class<add_ret_data<t_in1, t_in2>>>>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using sub_ret = add_ret<t_in1, decltype(-declval<t_in2>())>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using mul_ret = tcond_basic<is_intu_cpt<t_in1> && is_intu_cpt<t_in2>
		, tcond_basic<is_intp_cpt<t_in1> == is_intp_cpt<t_in2>, intp_class<mul_ret_data<t_in1, t_in2>>
		, intn_class<mul_ret_data<t_in1, t_in2>>>, ints_class<mul_ret_data<t_in1, t_in2>>>;

		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using div_ret = tcond_basic<is_intu_cpt<t_in1> && is_intu_cpt<t_in2>
		, tcond_basic<is_intp_cpt<t_in1> == is_intp_cpt<t_in2>, intp_class<typename t_in1::int_class::type>
		, intn_class<typename t_in1::int_class::type>>, ints_class<typename t_in1::int_class::type>>;

		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using modup_ret = intn_class<typename t_in2::int_class::type>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using moddw_ret = intp_class<typename t_in2::int_class::type>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using modtz_ret = tcond_basic<is_ints_cpt<t_in2>
		, ints_class<typename t_in2::int_class::type>, tcond_basic<is_intp_cpt<t_in2>
		, intp_class<typename t_in2::int_class::type>, intn_class<typename t_in2::int_class::type>>>;
		template<is_int_cpt t_in1, is_int_cpt t_in2>
		using modaz_ret = tcond_basic<is_ints_cpt<t_in2>
		, ints_class<typename t_in2::int_class::type>, tcond_basic<is_intp_cpt<t_in2>
		, intn_class<typename t_in2::int_class::type>, intp_class<typename t_in2::int_class::type>>>;
	public:
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator+(this t_self self, t_in in) -> add_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator+=(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator-(this t_self self, t_in in) -> sub_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator-=(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator*(this t_self self, t_in in) -> mul_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator*=(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator/(this t_self self, t_in in) -> div_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator/=(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divup(this t_self self, t_in in) -> div_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divup_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divdw(this t_self self, t_in in) -> div_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divdw_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divtz(this t_self self, t_in in) -> div_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divtz_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divaz(this t_self self, t_in in) -> div_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto divaz_a(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator%(this t_self self, t_in in) -> boo;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modup(this t_self self, t_in in) -> modup_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modup_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto moddw(this t_self self, t_in in) -> moddw_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto moddw_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modtz(this t_self self, t_in in) -> modtz_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modtz_a(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modaz(this t_self self, t_in in) -> modaz_ret<t_self, t_in>;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto modaz_a(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator<<(this t_self self, t_in in) -> t_self;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator<<=(this t_self& self, t_in in) -> t_self&;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator>>(this t_self self, t_in in) -> t_self;
		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto operator>>=(this t_self& self, t_in in) -> t_self&;

		template<is_int_cpt t_self, is_int_cpt t_in>
		constexpr auto pow(this t_self self, t_in in) -> t_self;
	protected:
		type m_data{};

		template<is_basic_intp_cpt u_type>
		friend class int_class;
		template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
		friend class intu_class;
		template<typename u_type>
		friend class intp_class;
		template<is_basic_intp_cpt u_type>
		friend class intn_class;
		template<is_basic_intp_cpt u_type>
		friend class ints_class;
	};
}

#endif