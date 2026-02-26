#ifndef COMPILERCPP_LIB_GUI_STU
#define COMPILERCPP_LIB_GUI_STU

#include <cstdint>

struct pos_2D {
    std::uint64_t x{}, y{};
};

struct size_2D {
    std::uint64_t x{}, y{};
};

struct color {
    float R{}, G{}, B{};
};

#endif