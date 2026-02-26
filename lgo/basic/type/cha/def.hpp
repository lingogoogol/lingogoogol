#ifndef LGO_BASIC_TYPE_CHA_DEF
#define LGO_BASIC_TYPE_CHA_DEF

#include "decl.hpp"
#include "basic_decl.hpp"
#include "alias.hpp"
#include "cpt.hpp"
#include "literal_decl.hpp"

#include "../boo/.hpp"
#include "../lgo/.hpp"
#include "../int/p/alias.hpp"
#include "../int/s/alias.hpp"

LGO_EXPORT namespace lgo {
    template<is_basic_cha_cpt t_type>
    class cha: public lgo {
    private:
        using type = t_type;
        
        template<basic_literal_str t_in>
        constexpr static auto make() -> cha;
    public:
        template<typename t_in>
        constexpr static auto size(const t_in&) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content, size_mem<>>;
        
		template<is_cha_cpt, is_str_cpt u_in>
		constexpr static auto str_get(const u_in& in) -> cha;
		template<is_str_cpt u_out>
		constexpr static auto str_pad(cha in, u_out& out) -> void;
		template<is_cha_cpt, is_mem_cpt u_in>
		constexpr static auto mem_get(const u_in& in) -> cha;
        template<is_mem_cpt u_out>
        constexpr static auto mem_pad(cha in, u_out& out) -> void;

        constexpr cha();
        constexpr explicit cha(type in);
        template<is_cha_cpt t_in>
        constexpr cha(t_in in);

        constexpr auto operator=(const cha& in) & -> cha& = default;

        constexpr auto basic() const -> type;

        template<is_cha_cpt t_in>
        constexpr auto operator<=>(t_in in) const -> intsc;

        template<is_int_cpt t_in>
        constexpr auto operator+(t_in in) const -> cha;
        template<is_int_cpt t_in>
        constexpr auto operator-(t_in in) const -> cha;

        template<basic_literal_str t_in>
        constexpr static cha val{ make<t_in>() };
    private:
        type m_data{};

        template<is_basic_cha_cpt u_type>
        friend class cha;

        template<basic_literal_str u_in>
        friend constexpr auto literal::operator""_cha8() -> cha8;
        template<basic_literal_str u_in>
        friend constexpr auto literal::operator""_cha16() -> cha16;
        template<basic_literal_str u_in>
        friend constexpr auto literal::operator""_cha32() -> cha32;
        template<basic_literal_str u_in>
        friend constexpr auto literal::operator""_chac() -> chac;
    };
}

#endif