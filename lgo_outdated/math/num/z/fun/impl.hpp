#ifndef LGO_MATH_NUM_Z_FUN_IMPL
#define LGO_MATH_NUM_Z_FUN_IMPL

LGO_EXPORT namespace lgo {
    template<is_numz_cpt t_out>
    auto rand() -> t_out {
		t_out out{};
        for (intpc i{ t_out::lit() }; (i <= t_out::big()).basic(); ++i) {
            out[i] = rand<typename t_out::type>();
        }
		return out;
    }
    
    template<is_numz_cpt t_out, is_numz_cpt t_max>
    auto rand(t_max max) -> t_out {
        if constexpr ((t_out::type::size(size_content{}) != t_max::type::size(size_content{})).basic()) {
            return rand<t_out>(t_out{ max });
        }
		t_out out{};
        for (intpc i{ t_out::lit() }; (i <= t_out::big()).basic(); ++i) {
            out[i] = rand<typename t_out::type>(max.part(i));
        }
		return out;
    }
}

#endif