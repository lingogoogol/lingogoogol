#ifndef COMPILERCPP_GUI_FREETYPE_FREETYPE
#define COMPILERCPP_GUI_FREETYPE_FREETYPE

#include "misc.hpp"

#include "../../lib/header.hpp"

class freetype_t {
public:
    freetype_t();

    auto handle_get() const -> FT_Library;
private:
    FT_Library m_handle{};
};

freetype_t::freetype_t() {
    FT_error(FT_Init_FreeType(&m_handle));
    return;
}

auto freetype_t::handle_get() const -> FT_Library {
    return m_handle;
}

#endif