import std;

auto main() -> int
{
    std::print("要生成的檔案大小：\n");
    std::string size_str{};
    std::getline(std::cin, size_str);
    std::uint64_t size{ std::stoull(size_str) };

    std::ofstream stream{ "filegen" };
    std::mt19937_64 random_engine{ std::random_device{}() };
    std::uniform_int_distribution distribution{ 0, 7 };
    for (std::uint64_t i{ 0 }; i < size; ++i)
    {
        stream.put(static_cast<unsigned char>(distribution(random_engine)));
    }
    return 0;
}
