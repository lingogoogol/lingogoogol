#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <fstream>
#include <map>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "surface.h"
#include "window_f.h"
#include "value_f.h"
#include "text_f.h"

enum class State {
    Init,
    Home,
    World
};

struct Setting {
    unsigned int start_width;
    unsigned int start_height;
    float acceleration;
    float max_speed;
    float angle_of_view;
    float friction;
    float cursor_sensitivity;
    glm::vec3 ambient_color;
    int text_resolution;
};

#endif