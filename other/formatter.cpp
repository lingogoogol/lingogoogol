#include <iostream>
#include <string>

auto new_line(int column) -> void {
    std::cout << "\r\n";
    for (int i{ 0 }; i < column; ++i) {
        std::cout << "|   ";
    }
    return;
}

auto main() -> int {
    std::string in{};
    std::getline(std::cin, in);
    int column{ 0 };
    for (int i{ 0 }; i < in.size(); ++i) {
        if (in[i] == '<' || in[i] == '(' || in[i] == '{') {
            std::cout << in[i];
            ++column;
            new_line(column);
        }
        else if (in[i] == '>' || in[i] == ')' || in[i] == '}') {
            --column;
            new_line(column);
            std::cout << in[i];
        }
        else if (in[i] == ',') {
            std::cout << in[i];
            new_line(column);
        }
        else {
            std::cout << in[i];
        }
    }
    while (true) {}
    return 0;
}