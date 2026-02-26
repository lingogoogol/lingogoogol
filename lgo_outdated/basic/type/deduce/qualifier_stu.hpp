#ifndef LGO_BASIC_TYPE_DEDUCE_QUALIFIER_STU
#define LGO_BASIC_TYPE_DEDUCE_QUALIFIER_STU

#include "../boo/cpt.hpp"

namespace lgo {
	template<typename t_in>
	struct qc_stu {
		using type = t_in;
        constexpr static cpt val{ false };
	};

	template<typename t_in>
	struct qc_stu<const t_in> {
		using type = t_in;
        constexpr static cpt val{ true };
	};
	
	template<typename t_in>
	struct qv_stu {
		using type = t_in;
        constexpr static cpt val{ false };
	};

	template<typename t_in>
	struct qv_stu<volatile t_in> {
		using type = t_in;
        constexpr static cpt val{ true };
	};
	
	template<typename t_in>
	struct qp_stu {
		using type = t_in;
        constexpr static cpt val{ false };
	};

	template<typename t_in>
	struct qp_stu<t_in*> {
		using type = t_in;
        constexpr static cpt val{ true };
	};
	
	template<typename t_in>
	struct qlr_stu {
		using type = t_in;
        constexpr static cpt val{ false };
	};

	template<typename t_in>
	struct qlr_stu<t_in&> {
		using type = t_in;
        constexpr static cpt val{ true };
	};
	
	template<typename t_in>
	struct qrr_stu {
		using type = t_in;
        constexpr static cpt val{ false };
	};

	template<typename t_in>
	struct qrr_stu<t_in&&> {
		using type = t_in;
        constexpr static cpt val{ true };
	};
}

#endif