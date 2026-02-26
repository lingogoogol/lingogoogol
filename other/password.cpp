#include <string>
#include <iostream>
#include <random>

int main() {
	std::string s{};
	bool number{ false };
	bool upper{ false };
	bool lower{ false };
	bool symbol{ false };
	std::mt19937 random{ static_cast<std::mt19937::result_type>(std::time(nullptr)) };
	std::uniform_int_distribution all_generator{ static_cast<int>('!'), static_cast<int>('~') };
	std::uniform_int_distribution number_generator{ static_cast<int>('0'), static_cast<int>('9') };
	std::uniform_int_distribution upper_generator{ static_cast<int>('A'), static_cast<int>('Z') };
	std::uniform_int_distribution lower_generator{ static_cast<int>('a'), static_cast<int>('z') };
	std::uniform_int_distribution symbol_generator{ 0,static_cast<int>(('~' - '!' + 1) -
		('9' - '0' + 1) - ('Z' - 'A' + 1) - ('z' - 'a' + 1) - 1) };
	for (int i{ 0 }; i < 9; ++i) {
		char ch{ static_cast<char>(all_generator(random)) };
		if (ch >= '0' && ch <= '9')
			number = true;
		else if (ch >= 'A' && ch <= 'Z')
			upper = true;
		else if (ch >= 'a' && ch <= 'z')
			lower = true;
		else
			symbol = true;
		s.push_back(ch);
	}
	for (int i{ 0 }; i < 3; ++i) {
		if (!number) {
			s.push_back(static_cast<char>(number_generator(random)));
			number = true;
		}
		else if (!upper) {
			s.push_back(static_cast<char>(upper_generator(random)));
			upper = true;
		}
		else if (!lower) {
			s.push_back(static_cast<char>(lower_generator(random)));
			lower = true;
		}
		else if (!symbol) {
			char ch{ static_cast<char>(symbol_generator(random)) };
			ch += '!';
			if (ch <= '/') {
				s.push_back(ch);
				continue;
			}
			ch += '9' - '/';
			if (ch <= '@') {
				s.push_back(ch);
				continue;
			}
			ch += 'Z' - '@';
			if (ch <= '`') {
				s.push_back(ch);
				continue;
			}
			ch += 'z' - '`';
			s.push_back(ch);
		}
		else {
			s.push_back(static_cast<char>(all_generator(random)));
		}
	}
	std::cout << s;
	while (true) {}
	return 0;
}