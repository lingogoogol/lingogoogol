#ifndef LGO_BASIC_FUN_BASIC_ALGO
#define LGO_BASIC_FUN_BASIC_ALGO

namespace lgo {
    template<typename t_in>
    constexpr auto basic_condval(boo cond, t_in true_val, t_in false_val) -> t_in {
        if (cond.basic()) {
            return true_val;
        }
        else {
            return false_val;
        }
    }
}

#endif