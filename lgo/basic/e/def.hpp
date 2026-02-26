#ifndef LGO_BASIC_E_DEF
#define LGO_BASIC_E_DEF

#include "../type/lgo/.hpp"
#include "../seq/str/md/def.hpp"

LGO_EXPORT namespace lgo {
	class e {
	public:
		e(strmdc message);

		auto operator=(const e&) & -> e& = default;

		auto message() const -> const strmdc&;
	private:
		strmdc m_message{};
	};
}

#endif