#ifndef LGO_BASIC_TYPE_LITERAL_STR_DEF
#define LGO_BASIC_TYPE_LITERAL_STR_DEF

#include "decl.hpp"

#include "../int/p/def.hpp"
#include "../int/p/alias.hpp"
#include "../cha/basic_decl.hpp"
#include "../cha/decl.hpp"
#include "../lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<is_intp_v_cpt t_len>
    class literal_str: public lgo {
    public:
        constexpr static auto len() -> intpc;
        
        template<basic_intpc u_len>
        constexpr literal_str(const basic_chac (&in)[u_len]);

		constexpr auto operator=(const literal_str& in) & -> literal_str& = default;

        template<typename t_self>
        constexpr auto operator[](this t_self self, intpc index) -> chac;

        basic_chac m_data[len().basic()]{};
    };
}

namespace lgo {
    template<basic_intpc t_len>
    constexpr inline intpc literal_str_len{ t_len - 1 };
}

LGO_EXPORT namespace lgo {
    template<basic_intpc u_len>
    literal_str(const basic_chac (&)[u_len]) -> literal_str<v<literal_str_len<u_len>>>;
}

#endif