#ifndef STRING_F_H
#define STRING_F_H

#include <string>

std::u32string to_u32string(std::string s);
std::string to_string(std::u32string s);

#endif