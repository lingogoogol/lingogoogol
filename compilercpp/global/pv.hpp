#ifndef COMPILERCPP_GLOBAL_PV
#define COMPILERCPP_GLOBAL_PV

#include <iostream>

class global_pv {
public:
    virtual ~global_pv() = default;
    virtual auto save(std::ostream& dest) -> void = 0;
};

#endif