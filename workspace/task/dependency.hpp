#ifndef DEPENDENCY
#define DEPENDENCY

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <exception>
#include <iterator>

#include "file.hpp"
#include "vector.hpp"
#include "command.hpp"

auto get_envpath() -> std::vector<std::filesystem::path> {
    std::string environment_variable{ std::getenv("INCLUDE") };
    return filter(map(split(environment_variable, ";") + cst::hpp_dir, canonical), [](const std::filesystem::path& in) {
        return std::filesystem::directory_entry{ in }.is_directory();
    });
}

auto find_circular(const std::vector<std::filesystem::path>& parent_file, const std::filesystem::path& file) -> void {
    if (std::find(parent_file.begin(), parent_file.end(), file) != parent_file.end()) {
        std::cout << "I found a circular dependency:" << std::endl;
        for (int i{ 0 }; i < parent_file.size(); ++i) {
            std::cout << parent_file[i] << std::endl;
        }
        std::cout << file << std::endl << std::endl;
    }
    return;
}

auto find_imports(const std::vector<std::filesystem::path>& envpath, const std::filesystem::path& filename
, const std::vector<std::filesystem::path>& parent, std::vector<std::filesystem::path>& processed) -> std::vector<std::string> {
    find_circular(parent, filename);
    if (std::find(processed.begin(), processed.end(), filename) != processed.end()) {
        return {};
    }
    processed.push_back(filename);

    std::smatch match{};
    std::string file{ get_file(filename) };
    std::filesystem::path basename{ get_basename(filename) };
    std::vector<std::string> out{};

    while (std::regex_search(file, match, std::regex{ "^(?:export )?import ([A-Za-z0-9_\\.:]+);$" })) {
        std::string to_push{ module_to_basename(match.str(1)) };
        if (to_push.starts_with("-")) {
            to_push = std::regex_replace(basename.string(), std::regex{ "-.+$" }, "") + to_push;
        }
        out.push_back(to_push);
        file = match.suffix().str();
    }

    const std::regex regex{ "\n\\s*#include\\s*[\"<](.*)[\">]" };
    auto path{ envpath + canonical(std::filesystem::path{ filename }.parent_path()) };
    std::sort(path.begin(), path.end());
    path.erase(std::unique(path.begin(), path.end()), path.end());

    for (std::sregex_iterator it{ file.begin(), file.end(), regex }; it != std::sregex_iterator{}; it++) {
        auto children{ filter(map(path, [it](const std::filesystem::path& in) {
            return in / it->str(1);
        }), [](const std::filesystem::path& in) {
            return std::filesystem::directory_entry{ in }.is_regular_file();
        }) };

        if (children.size() == 1) {
            out = out + find_imports(envpath, std::filesystem::canonical(children[0]), parent + filename, processed);
        }
        else if (children.size() > 1) {
            std::cout << "I have found multiple possible #include targets:" << std::endl;
            for (int i{ 0 }; i < children.size(); ++i) {
                std::cout << children[i] << std::endl;
            }
            std::cout << std::endl;
        }
        else {
            std::cout << "I cannot find any include targets: " << std::endl << it->str(1) << std::endl
            << filename.string() << std::endl << std::endl;
        }
    }
    return out;
}

auto update_dependency(const std::filesystem::path& filename, std::vector<std::string> imports) -> void {
    std::filesystem::path root{ get_root(filename) };
    std::filesystem::path basename{ get_basename(filename) };
    std::vector<std::string> import_file{ get_imports(root, basename.string()) };

    std::vector<std::string> extra{ difference(import_file, imports) };
    for (int i{ 0 }; i < extra.size(); ++i) {
        std::filesystem::path export_filename{ module_to_export_filename(root, extra[i]) };
        set_file(export_filename, std::regex_replace(get_file(export_filename), std::regex{ extra[i] }, ""));
    }
    
    std::vector<std::string> missing{ difference(imports, import_file) };
    for (int i{ 0 }; i < missing.size(); ++i) {
        std::filesystem::path export_filename{ module_to_export_filename(root, missing[i]) };
        set_file(export_filename, get_file(export_filename) + missing[i] + '\n');
    }
    
    std::string import_file_s{};
    for (int i{ 0 }; i < imports.size(); ++i) {
        import_file_s += imports[i] + '\n';
    }
    set_file(module_to_import_filename(root, basename.string()), import_file_s);
    return;
}

#endif