#ifndef COMPILERCPP_GUI_FREETYPE_MISC
#define COMPILERCPP_GUI_FREETYPE_MISC

#include <stacktrace>

#include "../../lib/header.hpp"
#include "../../lib/file.hpp"

auto FT_error(FT_Error result, std::stacktrace stack = std::stacktrace::current()) -> void;

auto FT_error(FT_Error result, std::stacktrace stack) -> void {
    if (result == FT_Err_Ok) {
        return;
    }
    log_file("FT_Error != FT_Err_Ok:\n");
    log_file("  FT_Error: " + std::to_string(result) + "\n");
    log_stacktrace(stack);
    log_file("\n");
    return;
}

#endif