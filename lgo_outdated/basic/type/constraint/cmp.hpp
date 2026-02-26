#ifndef LGO_BASIC_TYPE_CONSTRAINT_CMP
#define LGO_BASIC_TYPE_CONSTRAINT_CMP

#include "common.hpp"

#include "../int/s/alias.hpp"

LGO_EXPORT namespace lgo {
	template<typename t_in1, typename t_in2>
	concept is_comparable_cpt = requires(t_in1 in1, t_in2 in2) {
		{ in1 <=> in2 } -> is_same_cpt<intsc>;
	};
}

#endif