#ifndef LGO_BASIC_TYPE_PTR_MAIN_DC
#define LGO_BASIC_TYPE_PTR_MAIN_DC

#include "def.hpp"

LGO_EXPORT namespace lgo {
    template<typename t_from, typename t_to>
    concept is_dcable_cpt = is_ptr_cpt<t_from> && requires (t_from in) {
        in.template dc<t_to>();
    };
}

#endif