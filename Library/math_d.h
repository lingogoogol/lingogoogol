#ifndef LIBRARY_MATH_D_H
#define LIBRARY_MATH_D_H

#include <algorithm>
#include <time.h>
#include <chrono>

#include "math.h"

inline Plane::Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) :
    coefficient{ (point2 - point1) * (point3 - point1) }, constant{ glm::dot(coefficient, point1) } {
    return;
}

inline Line::Line(glm::vec3 origin_param, glm::vec3 point) :
    origin{ origin_param }, dir{ glm::normalize(point - origin_param) } {
    return;
}

inline std::int_fast8_t to_intfast8(char8_t ch) {
    return static_cast<std::int_fast8_t>(ch - '0');
}

inline char8_t to_char8(int i) {
    return static_cast<char8_t>('0' + i);
}

template<typename T1, int T2>
inline std::string to_string8(T1 in, int precision) {
    int pos{ 0 };
    while (in / std::pow(T2, pos) >= 1)
        pos++;
    if (pos > 0)
        pos--;
    std::string s{};
    if (in < 0) {
        s.push_back('-');
        in = -in;
    }
    T1 least{ static_cast<T1>(std::pow(T2, -precision)) / 2 };
    while ((in >= least || pos >= 0) && pos >= -precision) {
        if (pos == -1)
            s.push_back('.');
        int div{ static_cast<int>(std::trunc(in / std::pow(T2, pos))) };
        in -= static_cast<T1>(std::pow(T2, pos)) * div;
        s.push_back(to_char8(div));
        pos--;
    }
    if (in >= least) {
        bool after_point{ precision > 0 };
        for (int i{ static_cast<int>(s.size()) - 1 }; i >= 0; i--) {
            char ch{ s[i] };
            if (ch == '.')
                after_point = false;
            else if (ch != '0' + T2 - 1) {
                s[i]++;
                break;
            }
            else if (i == 0) {
                s[i] = '0';
                s.insert(0, 1, '1');
            }
            else if (s[static_cast<std::size_t>(i) - 1] == '-') {
                s[i] = '0';
                s.insert(1, 1, '1');
                break;
            }
            else
                s.pop_back();
        }
    }
    return s;
}

inline std::string to_string8(std::u32string s32) {
    std::string s8{};
    for (int i{ 0 }; i < s32.size(); i++)
        s8.push_back(static_cast<char>(s32[i]));
    return s8;
}

template<typename T1, int T2>
inline std::u32string to_string32(T1 in, int precision) {
     return std::u32string{};
}

inline std::u32string to_string32(std::string s8) {
    std::u32string s32{};
    for (int i{ 0 }; i < s8.size(); i++)
        s32.push_back(static_cast<char32_t>(s8[i]));
    return s32;
}

inline glm::vec3 Line::foot_of_perpendicular(glm::vec3 point) {
    return origin + dir * glm::dot(point - origin, dir) / glm::dot(dir, dir);
}

inline std::string get_time() {
    std::time_t time{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
    char cstring[26]{};
    ctime_s(cstring, sizeof(cstring), &time);
    std::string string{};
    for (int i{ 20 }; i < 24; i++)
        string.push_back(cstring[i]);
    string.push_back('.');
    switch (cstring[4]) {
    case 'J':
        switch (cstring[5]) {
        case 'a':
            string.push_back('1');
            break;
        case 'u':
            switch (cstring[6]) {
            case 'n':
                string.push_back('6');
                break;
            case 'l':
                string.push_back('7');
                break;
            }
            break;
        }
        break;
    case 'F':
        string.push_back('2');
        break;
    case 'M':
        switch (cstring[6]) {
        case 'r':
            string.push_back('3');
            break;
        case 'y':
            string.push_back('5');
            break;
        }
        break;
    case 'A':
        switch (cstring[5]) {
        case 'p':
            string.push_back('4');
            break;
        case 'u':
            string.push_back('8');
            break;
        }
        break;
    case 'S':
        string.push_back('9');
        break;
    case 'O':
        string.push_back('1');
        string.push_back('0');
        break;
    case 'N':
        string.push_back('1');
        string.push_back('1');
        break;
    case 'D':
        string.push_back('1');
        string.push_back('2');
        break;
    }
    string.push_back('.');
    if (cstring[8] != ' ')
        string.push_back(cstring[8]);
    string.push_back(cstring[9]);
    string.push_back(' ');
    for (int i{ 11 }; i < 19; i++)
        if (cstring[i] == ':')
            string.push_back('.');
        else
            string.push_back(cstring[i]);
    return string;
}

#endif