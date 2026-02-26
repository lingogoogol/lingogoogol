#include <algorithm>
#include <time.h>
#include <chrono>

#include "math.h"
#include "value_f.h"

Plane::Plane(glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) :
    coefficient{ (point2 - point1) * (point3 - point1) }, constant{ glm::dot(coefficient, point1) } {
    return;
}

Line::Line(glm::vec3 origin_param, glm::vec3 point) :
    origin{ origin_param }, dir{ glm::normalize(point - origin_param) } {
    return;
}

glm::vec3 Line::foot_of_perpendicular(glm::vec3 point) {
    return origin + dir * glm::dot(point - origin, dir) / glm::dot(dir, dir);
}

glm::vec3 to_cell_side(glm::vec3 begin, glm::vec3 dir) {
    glm::vec3 coefficient{};
    for (int i{ 0 }; i < 3; i++)
        coefficient[i] = dir[i] == 0 ? std::numeric_limits<float>::infinity() :
        (((dir[i] > 0 ? std::floor(begin[i] + 1) : std::ceil(begin[i] - 1)) - begin[i]) / dir[i]);
    float dist_to_cell_side{ std::min(coefficient.x, std::min(coefficient.y, coefficient.z)) };
    for (int i{ 0 }; i < 3; i++)
        begin[i] = coefficient[i] == dist_to_cell_side ? (dir[i] > 0 ? std::floor(begin[i] + 1) :
            std::ceil(begin[i] - 1)) : (begin[i] + dir[i] * dist_to_cell_side);
    return begin;
};

glm::ivec3 to_block(glm::ivec3 coord) {
    for (int i{ 0 }; i < 3; i++)
        coord[i] = coord[i] / constant::cell_num - (coord[i] >= 0 ? 0 : 
            (coord[i] % constant::cell_num == 0 ? 0 : 1));
    return coord;
}

glm::ivec3 to_coord_in_block(glm::ivec3 coord) {
    for (int i{ 0 }; i < 3; i++)
        coord[i] = coord[i] % constant::cell_num + (coord[i] >= 0 ? 0 : 1);
    return coord;
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