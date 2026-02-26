#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <regex>

#include "vector.hpp"
#include "file.hpp"
#include "dependency.hpp"
#include "command.hpp"
#include "cst.hpp"

auto compile(const std::string& configuration, const std::filesystem::path& filename, int flag) -> void {
    filetype type{ get_filetype(filename) };
    std::filesystem::path basename{ get_basename(filename) };

    std::string command{};
    add_arg(command, "\"" + find_msvc() + "cl.exe\"");
    add_args(command, cst::compiler_args);
    for (int i{ 0 }; i < cst::hpp_dir.size(); ++i) {
        add_arg(command, "/I\"" + cst::hpp_dir[i] + "\"");
    }
    add_arg(command, "/ifcSearchDir\"" + cst::ifch_dir + "\"");
    add_arg(command, "/ifcSearchDir\"" + cst::ifcm_dir + "\"");
    add_arg(command, "/Fd\"" + cst::pdbc_dir + basename.string() + ".pdb\"");
    add_arg(command, "/Fo\"" + cst::obj_dir + basename.string() + ".obj\"");

    if (configuration == "debug") {
        add_args(command, cst::debug_compiler_args);
    }
    else if (configuration == "release") {
        add_args(command, cst::release_compiler_args);
    }
    else {
        throw std::string{ "The compilation mode is not supported: " + configuration };
    }
    
    if (type != filetype::headerunit) {
        for (int i{ 0 }; i < cst::headerunits.size(); ++i) {
            add_arg(command, "/headerUnit:angle" + cst::headerunits[i] + '=' + cst::headerunits[i] + ".ifc");
        }
    }
    if (type == filetype::headerunit) {
        add_arg(command, "/ifcOutput\"" + cst::ifch_dir + "\"");
        add_arg(command, "/exportHeader");
    }
    if (type == filetype::interface) {
        add_arg(command, "/ifcOutput\"" + cst::ifcm_dir + "\"");
    }
    if (type == filetype::implementation) {
        add_arg(command, "/internalPartition");
    }

    std::cout << filename.string() << '\n';
    if (flag & preprocess) {
        std::string ppc_command{ command };
        add_arg(ppc_command, "/P");
        if (flag & hide_line) {
            add_arg(ppc_command, "/EP");
        }
        add_arg(ppc_command, '\"' + filename.string() + "\"");
        add_arg(ppc_command, "/Fi\"" + cst::ppc_dir + basename.string() + filetype_to_extension(type) + "\"");
        if (flag & show_command) {
            std::cout << ppc_command << '\n';
        }
        call(ppc_command, flag);
        add_arg(command, cst::ppc_dir + basename.string() + filetype_to_extension(type));
    }
    else {
        add_arg(command, '\"' + filename.string() + "\"");
    }

    call(command, flag);
    return;
}

auto link(const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::string>& imports) -> void {
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };

    std::string command{};
    add_arg(command, "\"" + find_msvc() + "link.exe\"");
    add_args(command, cst::linker_args);
    add_arg(command, "/ILK:\"" + cst::ilk_dir + basename.string() + ".ilk\"");
    add_arg(command, "/PDB:\"" + cst::pdbl_dir + basename.string() + ".pdb\"");
    add_arg(command, "/OUT:\"" + cst::exe_dir + basename.string() + ".exe\"");

    if (configuration == "debug") {
        add_args(command, cst::debug_linker_args);
    }

    if (target == "all") {
        add_arg(command, "/INCREMENTAL:NO");
    }

    add_obj(command, basename.string());

    for (int i{ 0 }; i < imports.size(); ++i) {
        add_objs(command, root, imports[i]);
    }

    call(command, flag);
    return;
}

auto process(const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed) -> void;

auto process_sub(const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::string>& sub_modules, const std::vector<std::filesystem::path>& parent
, std::vector<std::filesystem::path>& processed) -> void {
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path project{ get_project(filename) };

    for (int i{ 0 }; i < sub_modules.size(); ++i) {
        find_circular(parent, sub_modules[i]);
        if (std::find(processed.begin(), processed.end(), sub_modules[i]) == processed.end()) {
            process(configuration, module_to_filename(root, project, sub_modules[i]), target, flag, parent, processed);
        }
    }
    return;
}

auto process(const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed) -> void {
    std::string filename_s{ filename.string() };
    if (!std::filesystem::exists(filename)) {
        throw std::string{ "I could not find this file: " + filename_s };
    }
    
    filetype type{ get_filetype(filename) };
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };
    std::vector<std::filesystem::path> processed_header{};
    std::vector<std::string> imports{};
    if (target != "no_module") {
        imports = find_imports(get_envpath(), filename, {}, processed_header);
    }

    if (type == filetype::interface) {
        update_dependency(filename, imports);
    }
    
    if (target == "all") {
        processed.push_back(basename);
        process_sub(configuration, filename, target, flag, imports, parent + basename, processed);
    }
    else if (target == "no_module") {
        if (type != filetype::source) {
            throw "The target no_module must be used together with source file.";
        }
    }
    else if (target != "this") {
        throw "Compilation target must be \"all\" or \"this\" rather than" + target;
    }

    compile(configuration, filename, flag);
    if (type == filetype::source) {
        link(configuration, filename, target, flag, imports);
    }
    else if (type == filetype::interface) {
        process_sub(configuration, filename, "this", flag, get_exports(root, basename.string()), parent, processed);
    }
    return;
}

auto main(int argc, char** argv) -> int {
    try {
        if (argc < 4) {
            throw std::string{ "You didn't pass enough arguments." };
        }
        std::vector<std::filesystem::path> processed{};
        process(argv[1], argv[2], argv[3], get_flag(argv + 4, argc - 4), {}, processed);
    }
    catch (std::string description) {
        std::cout << "An error occurred: " << description << std::endl;
        return 1;
    }
    catch (std::exception e) {
        std::cout << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "An unknown error occurred." << std::endl;
        return 1;
    }
    return 0;
}