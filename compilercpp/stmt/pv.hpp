#ifndef COMPILERCPP_STMT_PV
#define COMPILERCPP_STMT_PV

class stmt_pv {
public:
    virtual ~stmt_pv() = default;
    virtual auto save(std::ostream& dest) -> void = 0;
};

#endif