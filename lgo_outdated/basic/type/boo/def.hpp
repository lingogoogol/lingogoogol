#ifndef LGO_BASIC_TYPE_BOO_DEF
#define LGO_BASIC_TYPE_BOO_DEF

#include "decl.hpp"
#include "cpt.hpp"

#include "../literal_str/basic.hpp"
#include "../tem/t/def.hpp"
#include "../tem/v/def.hpp"
#include "../lgo/def.hpp"
#include "../int/p/alias.hpp"
#include "../../size/stack/.hpp"
#include "../../size/content/.hpp"
#include "../../size/mem/decl.hpp"

LGO_EXPORT namespace lgo {
    class boo: public lgo {
    private:
        bool m_data{};
        
        template<basic_literal_str t_in>
        constexpr static auto make() -> boo;
    public:
        template<typename t_in>
        constexpr static auto size(const t_in&) -> intpc
        requires is_one_of_cpt<t_in, size_stack, size_content, size_mem<>>;
        
        constexpr boo();
        constexpr explicit boo(bool in);

        constexpr auto basic() const -> bool;
        constexpr auto cpt() const -> ::lgo::cpt;

        constexpr auto operator=(const boo& in) & -> boo& = default;

        constexpr auto operator==(boo in) const -> boo;
        constexpr auto operator!=(boo in) const -> boo;

        constexpr auto operator!() const -> boo;
        constexpr auto operator&&(boo in) const -> boo;
        constexpr auto operator||(boo in) const -> boo;

        template<basic_literal_str t_in>
        constexpr static boo val{ make<t_in>() };
        template<basic_literal_str t_in>
        using val_v = v<val<t_in>>;
        template<basic_literal_str t_in>
        using val_v_t = t<val_v<t_in>>;
    };
}

#endif