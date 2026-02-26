#ifndef LGO_BASIC_E_ERROR
#define LGO_BASIC_E_ERROR

#include "def.hpp"

LGO_EXPORT namespace lgo {
    auto error [[noreturn]] (e exc) -> void {
        throw exc;
    }
}

#endif