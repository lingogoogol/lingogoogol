#ifndef LGO_BASIC_TYPE_CONSTRAINT_COMMON
#define LGO_BASIC_TYPE_CONSTRAINT_COMMON

#include "../boo/cpt.hpp"

namespace lgo {
    template<typename, typename>
    struct is_same_cpt_stu {
        constexpr static cpt val{ false };
    };

    template<typename t_in>
    struct is_same_cpt_stu<t_in, t_in> {
        constexpr static cpt val{ true };
    };
}

LGO_EXPORT namespace lgo {
	template<typename t_in1, typename... t_in2>
	concept is_same_cpt = ((is_same_cpt_stu<t_in1, t_in2>::val) && ...);

	template<typename t_in1, typename... t_in2>
	concept is_one_of_cpt = (is_same_cpt<t_in1, t_in2> || ...);

	template<typename t_in, typename... t_bind>
	concept is_in_cpt = (t_bind::template type<t_in>::val.cpt() || ...);

	template<typename t_in1, typename... t_in2>
	concept is_constructible_cpt = requires(t_in2... in2) {
		{ t_in1{ in2... } };
	};

	template<typename t_in1>
	concept is_destructible_cpt = requires(t_in1 in1) {
		{ in1.~t_in1() };
	};

	template<typename t_in1, typename t_in2>
	concept is_assignable_cpt = requires(t_in1 in1, t_in2 in2) {
		{ in1 = in2 };
	};
	
	template<typename t_in>
	concept is_abstract_cpt = std::is_abstract_v<t_in>;

	template<typename t_in>
	concept is_instantiatable_cpt = !is_abstract_cpt<t_in> && !is_same_cpt<t_in, void>;
}

#endif