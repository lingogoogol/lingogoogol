#ifndef LGO_BASIC_TYPE_CONSTRAINT_BIND
#define LGO_BASIC_TYPE_CONSTRAINT_BIND

#include "boo.hpp"

#include "../tem/.hpp"

namespace lgo {
	template<typename... t_in>
	struct is_same_stu {
		using type = is_same_v<t_in...>;
	};
}

LGO_EXPORT namespace lgo {
	using is_same_bind = bind<tem<is_same_stu>>;
}

#endif