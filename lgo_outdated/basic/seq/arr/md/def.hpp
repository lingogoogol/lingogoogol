#ifndef LGO_BASIC_SEQ_ARR_MD_DEF
#define LGO_BASIC_SEQ_ARR_MD_DEF

#include "../subd/decl.hpp"
#include "../main/def.hpp"
#include "../../main/md/def.hpp"
#include "../../../type/lgo/.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_type>
    class arrmd: public lgo, public seqmd<t_type>, public arr {
    private:
        using base = seqmd<t_type>;
    public:
        using base::size;
        using arr::size;
        
        using base::seqmd;
        arrmd(const arrmd& in);
        template<is_seq_impl_cpt t_in>
        arrmd(const t_in& in)
        requires is_same_cpt<typename t_in::type, typename base::type>;
        arrmd(arrmd&&) = default;

        using base::operator=;
        auto operator=(const arrmd& in) & -> arrmd&;

        template<typename... t_opt>
        auto subms(t_opt... opt) -> arrsubs<arrmd>;
    };
}

#endif