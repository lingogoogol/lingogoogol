#ifndef WORKSPACE_TASK_FILE
#define WORKSPACE_TASK_FILE

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

#include "vector.hpp"
#include "cst.hpp"

enum class filetype {
    directory,
    headerunit,
    interface,
    implementation,
    source,
    hlsl,
    rc
};

std::filesystem::path canonical(const std::string& in) {
	try {
		return std::filesystem::canonical(in);
	}
	catch (std::exception e) {
		std::cout << "An error occurred when analyzing path \"" << in << "\": " << std::endl
		<< e.what() << std::endl << std::endl;
	}
	return std::filesystem::path{};
}

auto filetype_to_extension(filetype in) -> std::string {
    switch (in) {
    case filetype::headerunit:
        return ".hpp";
    case filetype::interface:
        return ".ixx";
    case filetype::source:
        return ".cpp";
    default:
        throw std::string{ "The file type is not supported." };
    }
}

auto module_to_basename(const std::string& module) -> std::string {
    return std::regex_replace(module, std::regex{ ":" }, "-");
}

auto check_filetype(const std::filesystem::path module_path, const std::string& module_name
, const std::string& slash) -> std::filesystem::path {
    std::filesystem::path cpp{ module_path / (module_name + slash + ".cpp") };
    std::filesystem::path ixx{ module_path / (module_name + slash + ".ixx") };
    bool cpp_exists{ std::filesystem::exists(cpp) };
    bool ixx_exists{ std::filesystem::exists(ixx) };
    if (cpp_exists && ixx_exists) {
        throw "This module name is not correct: " + module_name;
    }
    else if (cpp_exists) {
        return cpp;
    }
    else if (ixx_exists) {
        return ixx;
    }
    else {
        return "";
    }
}

auto module_to_obj_filename(const std::string& module) -> std::filesystem::path {
    return cst::obj_dir + module + ".obj";
}

auto module_to_filename(const std::filesystem::path& root, const std::filesystem::path& project
, const std::string& module) -> std::filesystem::path {
    std::string module_name{ std::regex_replace(module
    , std::regex{ "\\.|-" }, "/") };
    std::filesystem::path module_path{ root / "source" };
    if (!module_name.starts_with("lgo/")) {
        module_path /= project;
    }
    std::filesystem::path without_slash{ check_filetype(module_path, module_name, "") };
    std::filesystem::path with_slash{ check_filetype(module_path, module_name, "/") };
    bool without_slash_exists{ !without_slash.empty() };
    bool with_slash_exists{ !with_slash.empty() };
    if (without_slash_exists == with_slash_exists) {
        throw "This module name is not correct: " + module_name;
    }
    else if (without_slash_exists) {
        return without_slash;
    }
    else {
        return with_slash;
    }
}

auto module_to_import_filename(const std::filesystem::path& root, const std::string& module) -> std::filesystem::path {
    return root / "dependency" / (module + ".import");
}

auto module_to_export_filename(const std::filesystem::path& root, const std::string& module) -> std::filesystem::path {
    return root / "dependency" / (module + ".export");
}

auto get_file(const std::filesystem::path& filename) -> std::string {
    return (std::stringstream{} << std::ifstream{ filename }.rdbuf()).str();
}

auto set_file(const std::filesystem::path& filename, const std::string& file) -> void {
    std::ofstream{ filename, std::ofstream::trunc } << file;
    return;
}

auto get_filetype(const std::filesystem::path& filename) -> filetype {
    if (std::filesystem::is_directory(filename)) {
        return filetype::directory;
    }
    std::string filename_s{ filename.string() };
    if (filename_s.ends_with(".h") || filename_s.ends_with(".hpp")
    || !std::regex_search(filename_s, std::regex{ "\\.[^\\\\]+$" })) {
        return filetype::headerunit;
    }
    else if (filename_s.ends_with(".ixx")) {
        return filetype::interface;
    }
    else if (filename_s.ends_with(".cpp")) {
        if (get_file(filename).starts_with("module ")) {
            return filetype::implementation;
        }
        else {
            return filetype::source;
        }
    }
    else if (filename_s.ends_with(".hlsl")) {
        return filetype::hlsl;
    }
    else if (filename_s.ends_with(".rc")) {
        return filetype::rc;
    }
    else {
        std::cout << "The file extension is not supported: " + filename_s << std::endl;
        return filetype::headerunit;
    }
}

auto get_root(const std::filesystem::path& filename) -> std::filesystem::path {
    std::smatch match{};
    std::string filename_s{ filename.string() };
    std::regex_search(filename_s, match, std::regex{ "^(.*?[/\\\\]programming[/\\\\])" });
    return match.str(1);
}

auto get_project(const std::filesystem::path& filename) -> std::filesystem::path {
    std::smatch match{};
    std::string filename_s{ filename.string() };
    std::regex_search(filename_s, match
    , std::regex{ "[/\\\\]programming[/\\\\]source[/\\\\]([^/\\\\]+[/\\\\])" });
    return match.str(1);
}

auto get_basename(const std::filesystem::path& filename) -> std::filesystem::path {
    std::smatch match{};
    if (get_filetype(filename) == filetype::interface || get_filetype(filename) == filetype::implementation) {
        std::string file{ get_file(filename) };
        std::regex_search(file, match, std::regex{ "^(?:export )?module (.*?);" });
        return module_to_basename(match.str(1));
    }
    else {
        return filename.stem();
    }
}

auto get_ports(const std::filesystem::path& filename) -> std::vector<std::string> {
    std::vector<std::string> out{};
    std::string to_search{ get_file(filename) };
    std::smatch match{};
    while (std::regex_search(to_search, match, std::regex{ "^.+$" })) {
        out.push_back(match.str());
        to_search = match.suffix().str();
    }
    return out;
}

auto get_imports(const std::filesystem::path& root, const std::string& module) -> std::vector<std::string> {
    return get_ports(module_to_import_filename(root, module));
}

auto get_exports(const std::filesystem::path& root, const std::string& module) -> std::vector<std::string> {
    return get_ports(module_to_export_filename(root, module));
}

#endif