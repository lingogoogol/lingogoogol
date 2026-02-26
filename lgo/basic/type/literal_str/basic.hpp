#ifndef LGO_BASIC_TYPE_LITERAL_STR_BASIC
#define LGO_BASIC_TYPE_LITERAL_STR_BASIC

#include "../int/p/basic.hpp"
#include "../cha/basic_decl.hpp"
#include "../tem/t/cpt.hpp"
#include "../tem/v/cpt.hpp"

namespace lgo {
    template<basic_intpc t_len>
    class basic_literal_str {
    public:
        constexpr static auto len() -> basic_intpc;
        
        constexpr basic_literal_str(const basic_chac (&in)[t_len]);

        template<typename t_self>
        constexpr auto operator[](this t_self&& self, basic_intpc in) -> auto;

        basic_chac m_data[t_len]{};
    };

    template<typename>
    struct is_basic_literal_str_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<basic_intpc t_len>
    struct is_basic_literal_str_cpt_stu<basic_literal_str<t_len>> {
        constexpr static cpt val{ true };
    };

    template<typename t_in>
    concept is_basic_literal_str_cpt = is_basic_literal_str_cpt_stu<t_in>::val;
    template<typename t_in>
    concept is_basic_literal_str_t_cpt = is_t_cpt<t_in> && is_basic_literal_str_cpt<typename t_in::type>;
    template<typename t_in>
    concept is_basic_literal_str_v_cpt = is_v_cpt<t_in> && is_basic_literal_str_cpt<typename t_in::type>;
    
    template<basic_intpc t_len>
    constexpr auto basic_literal_str<t_len>::len() -> basic_intpc {
        return t_len - 1;
    }

    template<basic_intpc t_len>
    constexpr basic_literal_str<t_len>::basic_literal_str(const basic_chac (&in)[t_len]) {
        for (basic_intpc i{ 0 }; i < t_len; ++i) {
            m_data[i] = in[i];
        }
        return;
    }

    template<basic_intpc t_len>
    template<typename t_self>
    constexpr auto basic_literal_str<t_len>::operator[](this t_self&& self, basic_intpc in) -> auto {
        return self.m_data[in];
    }
}

#endif