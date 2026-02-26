#ifndef COMPILERCPP_DATA_TYPE_MAIN
#define COMPILERCPP_DATA_TYPE_MAIN

#include <iostream>
#include <string>
#include <vector>

#include "../global/var.hpp"
#include "../global/pv.hpp"

class data_type: public global_pv {
private:
    std::string m_name{};
    std::vector<var> m_member{};
public:
    data_type(std::istream& src) {
        return;
    }

    auto name() const -> const std::string& {
        return m_name;
    }
};

#endif