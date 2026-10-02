module;

#include <ft2build.h>
#include FT_FREETYPE_H

export module external.FreeType;

constexpr auto FT_LOAD_RENDER_export{ FT_LOAD_RENDER };
#undef FT_LOAD_RENDER

export
{
    using ::FT_UInt;
    using ::FT_Library;
    using ::FT_Error;
    using ::FT_Face;

    using ::FT_Init_FreeType;
    using ::FT_New_Face;
    using ::FT_Done_Face;
    using ::FT_Load_Glyph;
    using ::FT_Set_Pixel_Sizes;
    using ::FT_Get_Char_Index;

    using ::FT_Err_Ok;
    
    constexpr auto FT_LOAD_RENDER{ FT_LOAD_RENDER_export };
}
