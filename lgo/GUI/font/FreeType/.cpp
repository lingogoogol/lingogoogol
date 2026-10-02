module lgo.GUI.font.FreeType;

import external.FreeType;

import lgo.io.file;

namespace lgo
{
    FreeType_t::FreeType_t()
    {
        FT_error(FT_Init_FreeType(&m_handle));
        return;
    }

    auto FreeType_t::handle_get() const -> FT_Library
    {
        return m_handle;
    }


    auto FT_error(FT_Error result) -> void
    {
        if (result == FT_Err_Ok)
        {
            return;
        }
        log_file("FT_Error != FT_Err_Ok:\n");
        log_file("  FT_Error: " + std::to_string(result) + "\n");
        log_file("\n");
        return;
    }
}
