#ifndef LGO_BASIC_SEQ_STR_MD_DEF
#define LGO_BASIC_SEQ_STR_MD_DEF

#include "decl.hpp"
#include "literal_decl.hpp"

#include "../main/def.hpp"
#include "../subd/decl.hpp"
#include "../../main/md/def.hpp"
#include "../../io/d/def.hpp"
#include "../../../type/cha/basic_decl.hpp"
#include "../../../type/lgo/.hpp"
#include "../../../type/literal_str/basic.hpp"

namespace lgo {
    template<is_cha_cpt t_type>
    class strmd: public lgo, public seqmd<t_type>, public str, public seqiod {
    private:
        using base = seqmd<t_type>;

        template<basic_literal_str u_in>
        static auto make() -> strmd;
    public:
        using base::size;
        using str::size;
        
        using base::seqmd;
        strmd(const strmd& in);
        template<is_seq_impl_cpt t_in>
        strmd(const t_in& in)
        requires is_same_cpt<typename t_in::type, typename base::type>;
        strmd(strmd&&) = default;

        using base::operator=;
        auto operator=(const strmd& in) & -> strmd&;
        auto operator=(strmd&& in) & -> strmd&;

        template<typename t_self, typename... t_opt>
        auto subcs(this t_self& self, t_opt... opt) -> strsubs<const t_self>;
        template<typename t_self, typename... t_opt>
        auto subms(this t_self& self, t_opt... opt) -> strsubs<t_self>;
        template<typename t_self, typename... t_opt>
        auto subcd(this t_self& self, t_opt... opt) -> strsubd<const t_self>;
        template<typename t_self, typename... t_opt>
        auto submd(this t_self& self, t_opt... opt) -> strsubd<t_self>;
    private:
        template<basic_literal_str t_in>
        friend auto literal::operator""_strmd8() -> strmd8;
        template<basic_literal_str t_in>
        friend auto literal::operator""_strmd16() -> strmd16;
        template<basic_literal_str t_in>
        friend auto literal::operator""_strmd32() -> strmd32;
        template<basic_literal_str t_in>
        friend auto literal::operator""_strmdc() -> strmdc;
    };
}

#endif