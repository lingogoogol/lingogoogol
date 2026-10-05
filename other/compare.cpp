import std;

import lgo.io;

auto main() -> int
{
    std::string file1_path_str{}, file2_path_str{};
    std::print("file1檔名：\n");
    std::getline(std::cin, file1_path_str);
    std::print("file2檔名：\n");
    std::getline(std::cin, file2_path_str);
    std::print("正在讀取file1...\n");
    std::vector<unsigned char> file1_arrd( lgo::get_file(file1_path_str) );
    std::print("file1讀取完成！\n");
    std::print("正在讀取file2...\n");
    std::vector<unsigned char> file2_arrd{ lgo::get_file(file2_path_str) };
    std::print("file2讀取完成！\n");
    if (file1_arrd.size() == file2_arrd.size())
    {
        std::print("確認檔案大小相等：{0}B\n", file1_arrd.size());
    }
    else
    {
        std::print("確認檔案大小不同。file1：{0}B，file2：{1}B\n", file1_arrd.size(), file2_arrd.size());
        return 0;
    }
    for (std::size_t i{ 0 }; i < file1_arrd.size(); ++i)
    {
        if (file1_arrd[i] != file2_arrd[i])
        {
            std::print("檔案第{0}位元組處不同。file1：{1}，file2：{2}\n", i, file1_arrd[i], file2_arrd[i]);
            return 0;
        }
    }
    std::print("檔案內容完全相同！\n");
    return 0;
}
