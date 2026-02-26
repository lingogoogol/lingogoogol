#ifndef LGO_BASIC_MATH_FUN_IMPL
#define LGO_BASIC_MATH_FUN_IMPL

namespace lgo {
	auto std_mt() -> std::mt19937_64& {
		static std::mt19937_64 mt{ std::random_device{}() };
		return mt;
	}
}

LGO_EXPORT namespace lgo {
	template<is_intp_cpt t_out>
	auto rand() -> t_out {
		return t_out{ std_mt()() };
	}

	template<is_intp_cpt t_out, is_intp_cpt t_max>
	auto rand(t_max max) -> t_out {
        if (max == t_out::max()) {
			return rand<t_out>();
        }
		t_out out{};
        const intpc diff{ max.sc<intpc>() + U"1"_intpc };
        const intpc interval{ t_out::max().divdw(diff) };
		do {
            out = rand<t_out>();
 		} while ((out >= interval * diff).basic());
		return out / interval;
	}
}

#endif