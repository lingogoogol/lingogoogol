#ifndef LGO_BASIC_SEQ_MEM_MS_DEF
#define LGO_BASIC_SEQ_MEM_MS_DEF

#include "decl.hpp"

#include "../main/def.hpp"
#include "../main/cpt.hpp"
#include "../../io/s/def.hpp"

LGO_EXPORT namespace lgo {
	template<is_intp_v_cpt t_len>
	class memms: public lgo, public seqms<byte, t_len>, public mem, public seqios {
	private:
		using base = seqms<byte, t_len>;
	public:
		constexpr memms() = default;
		template<is_mem_cpt t_in>
		constexpr memms(const t_in& in);

		using base::operator=;
		constexpr auto operator=(const memms& in) & -> memms&;

		template<is_memms_cpt t_in>
		constexpr auto operator+(const t_in& in) const -> memms<v<base::len() + t_in::len()>>;
		
        template<typename t_self, typename... t_opt>
		constexpr auto subms(this t_self& self, t_opt... opt) -> memsubs<t_self>;
        template<typename t_self, typename... t_opt>
		constexpr auto subcs(this t_self& self, t_opt... opt) -> memsubs<const t_self>;
	};
}

#endif