#include <fstream>
#include <cstdint>

auto main() -> int {
    std::ofstream stream2{ "1GBn" };
    for (std::uint64_t i{ 0 }; i < (1ULL << 30) / 16; ++i) {
        stream2.write("FEDCBA9876543210", 16);
    }
    return 0;
}