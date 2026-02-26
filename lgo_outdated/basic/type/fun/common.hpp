#ifndef LGO_BASIC_TYPE_FUN_COMMON
#define LGO_BASIC_TYPE_FUN_COMMON

LGO_EXPORT namespace lgo {
	template<typename t_in>
	auto declval() -> t_in&&;

	template<typename t_in>
	constexpr t_in* decay_arr(t_in* in) {
		return in;
	}
}

#endif