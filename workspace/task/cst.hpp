#ifndef CST
#define CST

#include <vector>
#include <string>
#include <filesystem>

namespace cst {
    std::vector<std::string> compiler_args{
        "/EHsc",
        "/volatile:iso",
        "/permissive-",
        "/std:c++latest",
        "/source-charset:utf-8",
        "/Zc:enumTypes",
        "/W4",
        "/options:strict",
        "/diagnostics:caret",
        "/c"
    };
    std::vector<std::string> debug_compiler_args{
        "/Zi",
        "/MDd"
    };
    std::vector<std::string> release_compiler_args{
        "/O2"
    };
    std::vector<std::string> linker_args{
        "/WX",
        "/INCREMENTAL:NO"
    };
    std::vector<std::string> debug_linker_args{
        "/DEBUG"
    };
    std::vector<std::string> headerunits{
        "cstddef",
        "cstdint",
        "type_traits",
        "string",
        "functional",
        "iostream"
    };
    std::vector<std::string> hpp_dir{
        "../external/header/",
        "source/"
    };
    std::string lib_dir{ "../external/library/" };
    std::string ifch_dir{ "middle/ifch/" };
    std::string ifcm_dir{ "middle/ifcm/" };
    std::string ppc_dir{ "middle/ppc/" };
    std::string obj_dir{ "middle/obj/" };
    std::string ilk_dir{ "middle/ilk/" };
    std::string pdbc_dir{ "middle/pdbc/" };
    std::string pdbl_dir{ "middle/pdbl/" };
    std::string exe_dir{ "output/" };
    std::filesystem::path compiled_filename{ "middle/compiled" };
    std::filesystem::path parent_filename{ "middle/parent" };
}

#endif