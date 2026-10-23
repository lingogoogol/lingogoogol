import std;

import lgo.io;

auto main() -> int
{
    std::string file1_path_str{}, file2_path_str{};
    std::print("file1檔名：\n");
    std::getline(std::cin, file1_path_str);
    std::print("file2檔名：\n");
    std::getline(std::cin, file2_path_str);
    std::uintmax_t file_size{ std::filesystem::file_size(file1_path_str) };
    std::uintmax_t file2_size{ std::filesystem::file_size(file2_path_str) };
    if (file_size == file2_size)
    {
        std::print("確認檔案大小相等：{0}B\n", file_size);
    }
    else
    {
        std::print("確認檔案大小不同。file1：{0}B，file2：{1}B\n", file_size, file2_size);
        return 0;
    }
    std::ifstream file1_stream{ lgo::create_ifstream(file1_path_str) },
        file2_stream{ lgo::create_ifstream(file2_path_str) };
    for (std::size_t i{ 0 }; i < file_size; ++i)
    {
        int data1{ file1_stream.get() }, data2{ file2_stream.get() };
        if (data1 != data2)
        {
            std::print("檔案第{0}位元組處不同。file1：{1}，file2：{2}\n", i, data1, data2);
            return 0;
        }
    }
    std::print("檔案內容完全相同！\n");
    return 0;
}
