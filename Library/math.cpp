#include <algorithm>
#include <time.h>
#include <chrono>

#include <Library/math.h>

Plane::Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) :
    coefficient{ (point2 - point1) * (point3 - point1) }, constant{ glm::dot(coefficient, point1) } {
    return;
}

Line::Line(glm::vec3 origin_param, glm::vec3 point) :
    origin{ origin_param }, dir{ glm::normalize(point - origin_param) } {
    return;
}

std::string to_string8(float f, int precision) {
    int digit{ 0 };
    while (f / std::pow(10, digit) >= 1)
        digit += 1;
    if (digit > 0)
        digit--;
    std::string s8{};
    if (f < 0)
        s8.push_back('-');
    while (digit >= 0) {
        int div{ static_cast<int>(std::trunc(f / std::pow(10, digit))) };
        f -= static_cast<float>(std::pow(10, digit)) * div;
        s8 += std::to_string(std::abs(div));
        digit--;
    }
    float least{ static_cast<float>(std::pow(10, -precision)) / 2 };
    if (std::abs(f) >= least) {
        s8.push_back('.');
        while (std::abs(f) >= least && digit > -precision) {
            int div{ static_cast<int>(std::trunc(f / std::pow(10, digit))) };
            f -= static_cast<float>(std::pow(10, digit)) * div;
            s8 += std::to_string(std::abs(div));
            digit--;
        }
        int div{ static_cast<int>(std::trunc(f / std::pow(10, digit))) };
        f -= static_cast<float>(std::pow(10, digit)) * div;
        if (std::abs(f) >= least)
            div++;
        if (div != 0)
            s8 += std::to_string(std::abs(div));
    }
    return s8;
}

std::string to_string8(double d, int precision) {
    int digit{ 0 };
    while (d / std::pow(10, digit) >= 1)
        digit += 1;
    if (digit > 0)
        digit--;
    std::string s8{};
    if (d < 0)
        s8.push_back('-');
    while (digit >= 0) {
        int div{ static_cast<int>(std::trunc(d / std::pow(10, digit))) };
        d -= std::pow(10, digit) * div;
        s8 += std::to_string(std::abs(div));
        digit--;
    }
    double least{ std::pow(10, -precision) / 2 };
    if (std::abs(d) >= least) {
        s8.push_back('.');
        while (std::abs(d) >= least && digit > -precision) {
            int div{ static_cast<int>(std::trunc(d / std::pow(10, digit))) };
            d -= std::pow(10, digit) * div;
            s8 += std::to_string(std::abs(div));
            digit--;
        }
        int div{ static_cast<int>(std::trunc(d / std::pow(10, digit))) };
        d -= std::pow(10, digit) * div;
        if (std::abs(d) >= least)
            div++;
        if (div != 0)
            s8 += std::to_string(std::abs(div));
    }
    return s8;
}

std::string to_string8(std::u32string s32) {
    std::string s8{};
    for (int i{ 0 }; i < s32.size(); i++)
        s8.push_back(static_cast<char>(s32[i]));
    return s8;
}

std::u32string to_string32(float f, int precision) {
    std::u32string s32{};
    return s32;
}

std::u32string to_string32(double d, int precision) {
    std::u32string s32{};
    return s32;
}

std::u32string to_string32(std::string s8) {
    std::u32string s32{};
    for (int i{ 0 }; i < s8.size(); i++)
        s32.push_back(static_cast<char32_t>(s8[i]));
    return s32;
}

glm::vec3 Line::foot_of_perpendicular(glm::vec3 point) {
    return origin + dir * glm::dot(point - origin, dir) / glm::dot(dir, dir);
}

std::string get_time() {
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