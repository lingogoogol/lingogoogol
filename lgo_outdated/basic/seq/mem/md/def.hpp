#ifndef LGO_BASIC_SEQ_MEM_MD_DEF
#define LGO_BASIC_SEQ_MEM_MD_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../../main/md/def.hpp"
#include "../../io/d/def.hpp"

namespace lgo {
	class memmd: public lgo, public seqmd<byte>, public mem, public seqiod {
    private:
        using base = seqmd<byte>;
	public:
        using base::size;
        using mem::size;
        
        using base::seqmd;
		memmd(const memmd& in);
        template<is_seq_impl_cpt t_in>
        memmd(const t_in& in)
        requires is_same_cpt<typename t_in::type, byte>;
        memmd(memmd&&) = default;

		using base::operator=;
		auto operator=(const memmd& in) & -> memmd&;
		auto operator=(memmd&& in) & -> memmd&;

        template<typename t_self, typename... t_opt>
        auto subcs(this t_self& self, t_opt... opt) -> memsubs<const t_self>;
        template<typename t_self, typename... t_opt>
        auto subms(this t_self& self, t_opt... opt) -> memsubs<t_self>;
        template<typename t_self, typename... t_opt>
        auto subcd(this t_self& self, t_opt... opt) -> memsubd<const t_self>;
        template<typename t_self, typename... t_opt>
        auto submd(this t_self& self, t_opt... opt) -> memsubd<t_self>;
	};
}

#endif