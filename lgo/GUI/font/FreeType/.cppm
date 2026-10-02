export module lgo.GUI.font.FreeType;

import std;

import external.FreeType;

export namespace lgo
{
    class FreeType_t
    {
    public:
        FreeType_t();

        auto handle_get() const -> FT_Library;
    private:
        FT_Library m_handle{};
    };
    
    auto FT_error(FT_Error result) -> void;
}
