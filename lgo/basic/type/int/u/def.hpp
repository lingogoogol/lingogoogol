#ifndef LGO_BASIC_TYPE_INT_U_DEF
#define LGO_BASIC_TYPE_INT_U_DEF

#include "../main/cpt.hpp"
#include "../main/def.hpp"
#include "../main/opt/cpt/endian.hpp"
#include "../p/basic.hpp"
#include "../p/alias.hpp"
#include "../p/literal_decl.hpp"
#include "../n/literal_decl.hpp"
#include "../n/alias.hpp"
#include "../../literal_str/basic.hpp"
#include "../../tem/t/def.hpp"
#include "../../tem/tem/def.hpp"
#include "../../tem/bind/cpt.hpp"
#include "../../tem/bind/def.hpp"
#include "../../../seq/mem/ms/decl.hpp"
#include "../../../bind/iden.hpp"
#include "../../../size/mem/cpt.hpp"

namespace lgo {
    template<is_basic_intp_cpt t_type, is_intu_cpt t_self>
    class intu_class: public int_class<t_type> {
    private:
        using base = int_class<t_type>;
    protected:
        using base::int_class;
		constexpr intu_class(const intu_class&) = delete;
        
		constexpr auto operator=(const intu_class&) -> intu_class& = delete;

        using base::size;
    public:
		template<is_intu_cpt u_out, is_mem_cpt u_in>
		constexpr static auto mem_get(const u_in& in) -> u_out;
        template<is_intu_cpt u_in, is_mem_cpt u_out>
        constexpr static auto mem_pad(u_in in, u_out& out) -> void;
        template<is_intu_cpt u_in, is_mem_cpt u_out, is_int_endian_cpt u_endian>
        constexpr static auto mem_pad(u_in in, u_out& out, u_endian) -> void;
    private:
        template<typename t_intpc, is_basic_literal_str_v_cpt t_in>
        struct intpc_val_stu {
            using type = t_intpc::template val_v<t_in::val>;
        };
        using intpc_val_bind = bind<tem<intpc_val_stu>>;
        constexpr static basic_literal_str basic_literal_str_0{ U"0" };
    public:
        template<typename t_in>
		constexpr static auto size(const t_in&) -> intpc
        requires is_size_mem_cpt<t_in> && ((t_in::len() == U"0"_intpc).cpt()
        || ((t_in::len() == U"1"_intpc).cpt() && is_int_endian_cpt<typename t_in::template type
        <typename suspend_bind<intpc_val_bind, intpc, v<basic_literal_str_0>>::template type<t_in>>>));
    private:
		template<is_intu_cpt u_self, is_intu_cpt u_in>
        struct less_ret_data_stu {
            using type = tcond_basic<(sizeof(typename u_self::intu_class::type) <= sizeof(typename u_in::intu_class::type))
            , typename u_self::intu_class::type, typename u_in::intu_class::type>;
        };

		template<is_intu_cpt u_self, is_intu_cpt u_in>
        struct bigger_ret_data_stu {
            using type = tcond_basic<(sizeof(typename u_self::intu_class::type) >= sizeof(typename u_in::intu_class::type))
            , typename u_self::intu_class::type, typename u_in::intu_class::type>;
        };

		template<is_bind_cpt t_bind, is_intu_cpt u_self, is_intu_cpt u_in>
		struct bop_ret_stu {
            using type = tcond_basic<is_intp_cpt<u_self> && is_intp_cpt<u_in>
            , intp_class<typename t_bind::template type<u_self, u_in>>
            , intn_class<typename t_bind::template type<u_self, u_in>>>;
        };

        using less_ret_data = bind<tem<less_ret_data_stu>>;
        using bigger_ret_data = bind<tem<bigger_ret_data_stu>>;
        using bop_ret = bind<tem<bop_ret_stu>>;

        template<is_intu_cpt u_self, is_intu_cpt u_in>
        using band_ret = bop_ret::template type<less_ret_data, u_self, u_in>;
        template<is_intu_cpt u_self, is_intu_cpt u_in>
        using bor_ret = bop_ret::template type<bigger_ret_data, u_self, u_in>;
        template<is_intu_cpt u_self, is_intu_cpt u_in>
        using bxor_ret = bop_ret::template type<bigger_ret_data, u_self, u_in>;
    public:
		template<is_intu_cpt u_self, is_intu_cpt u_in, is_intu_cpt u_out>
		constexpr auto sub_overflow_o(this u_self self, u_in in, u_out& out) -> void;

		template<is_intu_cpt u_self>
		constexpr auto operator~(this u_self self) -> u_self;
		template<is_intu_cpt u_self>
		constexpr auto bnot_a(this u_self& self) -> u_self&;

		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator&(this u_self self, u_in in) -> band_ret<u_self, u_in>;
		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator&=(this u_self& self, u_in in) -> u_self&;

		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator|(this u_self self, u_in in) -> bor_ret<u_self, u_in>;
		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator|=(this u_self& self, u_in in) -> u_self&;

		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator^(this u_self self, u_in in) -> bxor_ret<u_self, u_in>;
		template<is_intu_cpt u_self, is_intu_cpt u_in>
		constexpr auto operator^=(this u_self& self, u_in in) -> u_self&;
    private:
    	template<basic_literal_str u_in>
		constexpr static auto make() -> t_self;
        
		template<basic_literal_str t_in>
		friend constexpr auto literal::operator""_intp8() -> intp8;
		template<basic_literal_str t_in>
		friend constexpr auto literal::operator""_intp16() -> intp16;
		template<basic_literal_str t_in>
		friend constexpr auto literal::operator""_intp32() -> intp32;
		template<basic_literal_str t_in>
		friend constexpr auto literal::operator""_intp64() -> intp64;
		template<basic_literal_str t_in>
		friend constexpr auto literal::operator""_intpc() -> intpc;

        template<basic_literal_str t_in>
        friend constexpr auto literal::operator""_intn8() -> intn8;
        template<basic_literal_str t_in>
        friend constexpr auto literal::operator""_intn16() -> intn16;
        template<basic_literal_str t_in>
        friend constexpr auto literal::operator""_intn32() -> intn32;
        template<basic_literal_str t_in>
        friend constexpr auto literal::operator""_intn64() -> intn64;
        template<basic_literal_str t_in>
        friend constexpr auto literal::operator""_intnc() -> intnc;
    public:
        template<basic_literal_str u_in>
        constexpr static t_self val{ make<u_in>() };
        template<basic_literal_str u_in>
        using val_v = v<val<u_in>>;
    };
}

#endif