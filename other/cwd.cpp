#include <filesystem>
#include <iostream>

auto main() -> int {
    std::cout << std::filesystem::current_path() << '\n';
    return 0;
}