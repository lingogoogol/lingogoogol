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

auto compile(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, int flag, std::vector<std::string>& object) -> void {
    filetype type{ get_filetype(filename) };
    std::filesystem::path basename{ get_basename(filename) };

    std::string command{};
    add_arg(command, "\"" + find_msvc().string() + "cl.exe\"");
    add_args(command, cst::compiler_args);
    if (ver != "11") {
        add_arg(command, "/std:c++" + ver);
    }
    for (int i{ 0 }; i < cst::hpp_dir.size(); ++i) {
        add_arg(command, "/I\"" + cst::hpp_dir[i] + "\"");
    }
    add_arg(command, "/ifcSearchDir\"" + cst::ifch_dir + "\"");
    add_arg(command, "/ifcSearchDir\"" + cst::ifcm_dir + "\"");
    add_arg(command, "/Fd\"" + cst::pdbc_dir + basename.string() + ".pdb\"");
    std::string obj_path{ cst::obj_dir + std::regex_replace(filename.string(), std::regex{ "[:\\\\]" }, "_") + ".obj" };
    add_arg(command, "/Fo\"" + obj_path + "\"");
    object.push_back(obj_path);

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
            add_arg(command, "/headerUnit:angle" + cst::headerunits[i] + "=" + cst::headerunits[i] + ".ifc");
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
        add_arg(ppc_command, "\"" + filename.string() + "\"");
        add_arg(ppc_command, "/Fi\"" + cst::ppc_dir + basename.string() + filetype_to_extension(type) + "\"");
        if (flag & show_command) {
            std::cout << ppc_command << '\n';
        }
        call(ppc_command, flag);
        add_arg(command, cst::ppc_dir + basename.string() + filetype_to_extension(type));
    }
    else {
        add_arg(command, "\"" + filename.string() + "\"");
    }

    call(command, flag);
    return;
}

auto link(const std::string& configuration, std::vector<std::string>& object, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::string>& imports) -> void {
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };

    std::string command{};
    add_arg(command, "\"" + find_msvc().string() + "link.exe\"");
    add_args(command, cst::linker_args);
    add_arg(command, "/ILK:\"" + cst::ilk_dir + basename.string() + ".ilk\"");
    add_arg(command, "/PDB:\"" + cst::pdbl_dir + basename.string() + ".pdb\"");
    std::string exe_path{ "\"" + cst::exe_dir + basename.string() + ".exe\"" };
    add_arg(command, "/OUT:" + exe_path);
    for (std::size_t i{ 0 }; i < cst::lib_dir.size(); ++i) {
        add_arg(command, "/LIBPATH:" + cst::lib_dir[i]);
    }
    std::filesystem::remove(exe_path);

    if (configuration == "debug") {
        add_args(command, cst::debug_linker_args);
    }
    if (target == "all") {
        add_arg(command, "/INCREMENTAL:NO");
    }

    for (std::size_t i{ 0 }; i < object.size(); ++i) {
        add_arg(command, "\"" + object[i] + "\"");
    }
    for (int i{ 0 }; i < imports.size(); ++i) {
        add_objs(command, root, imports[i]);
    }
    if (std::filesystem::exists("middle/res/" + basename.string() + ".res")) {
        add_arg(command, "\"middle/res/" + basename.string() + ".res\"");
    }

    call(command, flag);
    return;
}

auto process_CPP(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed) -> void;

auto process_CPP_sub(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::string>& sub_modules, const std::vector<std::filesystem::path>& parent
, std::vector<std::filesystem::path>& processed) -> void {
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path project{ get_project(filename) };

    for (int i{ 0 }; i < sub_modules.size(); ++i) {
        find_circular(parent, sub_modules[i]);
        if (std::find(processed.begin(), processed.end(), sub_modules[i]) == processed.end()) {
            process_CPP(ver, configuration, module_to_filename(root, project, sub_modules[i]), target, flag, parent, processed);
        }
    }
    return;
}

auto process_CPP1(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed
, std::vector<std::string>& object, std::vector<std::string>& imports) -> void {
    filetype type{ get_filetype(filename) };
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };
    std::vector<std::filesystem::path> processed_header{};
    if (target != "no_module") {
        imports = find_imports(get_envpath(), filename, {}, processed_header);
    }

    if (type == filetype::interface) {
        update_dependency(filename, imports);
    }
    
    if (target == "all") {
        processed.push_back(basename);
        process_CPP_sub(ver, configuration, filename, target, flag, imports, parent + basename, processed);
    }
    else if (target == "no_module") {
        if (type != filetype::source) {
            throw "The target no_module must be used together with source file.";
        }
    }
    else if (target != "this") {
        throw "Compilation target must be \"all\" or \"this\" rather than" + target;
    }

    compile(ver, configuration, filename, flag, object);
    return;
}

auto process_CPP_directory(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed
, std::vector<std::string>& object, std::vector<std::string>& imports) -> void {
    for (std::filesystem::directory_iterator file{ filename }; file != std::filesystem::directory_iterator{}; ++file) {
        if (file->is_directory()) {
            process_CPP_directory(ver, configuration, file->path(), target, flag, parent, processed, object, imports);
        }
        if (file->path().extension() == ".cpp") {
            process_CPP1(ver, configuration, file->path(), target, flag, parent, processed, object, imports);
        }
    }
    return;
}

auto process_CPP(const std::string& ver, const std::string& configuration, const std::filesystem::path& filename, const std::string& target
, int flag, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed) -> void {
    std::string filename_s{ filename.string() };
    if (!std::filesystem::exists(filename)) {
        throw std::string{ "I could not find this file: " + filename_s };
    }
    
    filetype type{ get_filetype(filename) };
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };
    std::vector<std::string> imports{};
    std::vector<std::string> object{};
    if (type == filetype::directory) {
        process_CPP_directory(ver, configuration, filename, target, flag, parent, processed, object, imports);
    }
    else {
        process_CPP1(ver, configuration, filename, target, flag, parent, processed, object, imports);
    }
    if (type == filetype::source || type == filetype::directory) {
        link(configuration, object, filename, target, flag, imports);
    }
    else if (type == filetype::interface) {
        process_CPP_sub(ver, configuration, filename, "this", flag, get_exports(root, basename.string()), parent, processed);
    }
    return;
}

auto process_HLSL(const std::string& configuration, const std::filesystem::path& filename, const std::string& type, int flag) -> void {
    const std::string model{ "6_0" };
    const std::string version{ "2021" };

    std::string command{};
    add_arg(command, "\"" + find_dxc().string() + "dxc.exe\"");
    add_arg(command, "-E " + type);
    std::filesystem::path basemane{ get_basename(filename) };
    add_arg(command, "-Fd \"middle/pdbs/" + basemane.string() + ".pdb\"");
    add_arg(command, "-Fo \"output/" + basemane.string() + ".cso\"");
    add_arg(command, "-HV " + version);
    std::string type_profile{};
    if (type == "vertex") {
        type_profile = "vs";
    }
    else if (type == "pixel") {
        type_profile = "ps";
    }
    else {
        throw std::string{ "shader_type" };
    }
    add_arg(command, "-T " + type_profile + "_" + model);
    if (configuration == "debug") {
        add_arg(command, "-Zi");
    }
    else if (configuration != "release") {
        throw std::string{ "configuration" };
    }
    add_arg(command, "\"" + filename.string() + "\"");
    call(command, flag);
    return;
}

auto process_RC(const std::filesystem::path& filename, int flag) -> void {
    std::string command{};
    add_arg(command, "RC");
    add_arg(command, "-v");
    std::filesystem::path basemane{ get_basename(filename) };
    add_arg(command, "-fo \"middle/res/" + basemane.string() + ".res\"");
    add_arg(command, "\"" + filename.string() + "\"");
    call(command, flag);
    return;
}

auto main(int argc, char** argv) -> int {
    try {
        std::vector<std::filesystem::path> processed{};
        std::string language{ argv[1] };
        if (language == "C++") {
            if (argc < 4) {
                throw std::string{ "You didn't pass enough arguments." };
            }
            process_CPP(argv[2], argv[3], argv[4], argv[5], get_option(argv + 6, argc - 6), {}, processed);
        }
        else if (language == "HLSL") {
            process_HLSL(argv[2], argv[3], argv[4], get_option(argv + 5, argc - 5));
        }
        else if (language == "RC") {
            process_RC(argv[2], get_option(argv + 3, argc - 3));
        }
        else {
            throw std::string{ "language" };
        }
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