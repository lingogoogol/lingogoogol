#ifndef COMMAND
#define COMMAND

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <regex>

#include "file.hpp"

constexpr inline int hide_line{ 0b00000001 };
constexpr inline int show_command{ 0b00000010 };
constexpr inline int preprocess{ 0b00000100 };

auto get_flag(char** flag, int size) -> int {
    int out{ 0b00000000 };
    for (int i{ 0 }; i < size; ++i) {
        std::string arg_string{ flag[i] };
        if (arg_string == "hide_line") {
            out |= hide_line;
        }
        else if (arg_string == "show_command") {
            out |= show_command;
        }
        else if (arg_string == "preprocess") {
            out |= preprocess;
        }
        else {
            throw std::string{ "The argument is not recognized." };
        }
    }
    return out;
}

auto find_msvc() -> std::string {
    bool compiler_exist{ false };
    std::string msvc_dir{};
    for (std::filesystem::directory_iterator i{ "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\" }
    ; i != std::filesystem::directory_iterator{}; ++i) {
        msvc_dir = i->path().string() + "\\bin\\Hostx64\\x64\\";
        if (std::filesystem::exists(msvc_dir)) {
            if (compiler_exist) {
                throw std::string{ "There are more possible path to the compiler." };
            }
            compiler_exist = true;
            break;
        }
    }
    if (!compiler_exist) {
        throw std::string{ "We cannot find the compiler." };
    }
    return msvc_dir;
}

auto call(const std::string& command, int flag) -> void {
    if (flag & show_command) {
        std::cout << command << '\n';
    }
    if (std::system(('\"' + command + '\"').c_str()) != 0) {
        throw std::string{ "The compilation failed." };
    }
    return;
}

auto add_arg(std::string& command, const std::string& arg) -> void {
    command += arg + ' ';
    return;
}

auto add_args(std::string& command, const std::vector<std::string>& args) {
    for (int i{ 0 }; i < args.size(); ++i) {
        add_arg(command, args[i]);
    }
    return;
}

auto add_obj(std::string& command, const std::string& module) -> void {
    add_arg(command, "\"" + module_to_obj_filename(module).string() + "\"");
    return;
}

auto add_objs(std::string& command, const std::filesystem::path& root, const std::string& module) -> void {
    if (std::regex_search(command, std::regex{ std::regex_replace
    (module_to_obj_filename(module).string(), std::regex{ "\\." }, "\\.") })) {
        return;
    }
    add_obj(command, module);
    std::string import_file{ get_file(module_to_import_filename(root, module)) };
    std::vector<std::string> imports{ get_imports(root, module) };
    for (int i{ 0 }; i < imports.size(); ++i) {
        add_objs(command, root, imports[i]);
    }
    return;
}

#endif