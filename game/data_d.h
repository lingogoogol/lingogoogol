#ifndef DATA_H
#define DATA_H

#include <string>
#include <map>
#include <vector>

#include <lgo/lib/glad.h>
#include <lgo/lib/glfw.h>
#include <lgo/lib/freetype.h>
#include <lgo/arr/s/.h>
#include <lgo/vec/s/.h>
#include <lgo/mat/s/.h>
#include <lgo/graphic/.h>
#include <lgo/text/.h>
#include <lgo/button/.h>
#include <lgo/holder_i.h>

#include "const_f.h"
#include "tile_f.h"
#include "block_f.h"
#include "surface_f.h"
#include "data_f.h"

enum class State {
    Wait,
    Home,
    Environment,
    World,
    World_init
};

struct Shader {
    unsigned int text{};
    unsigned int button{};
    unsigned int transparent_cell{};
    unsigned int transparent_object{};
    unsigned int tile_ambient{};
    unsigned int tile_dir{};
    unsigned int tile_point{};
    unsigned int tile_spot{};
    unsigned int opaque_cell{};
    unsigned int opaque_object{};
};

class Data {
public:
    int text_resolution{};
    GLFWwindow* main_window{};
    lgo::file<char>* logfile{};
    float current_time{};
    float last_time{};
    int last_second{};
    int frame_count{};
    float frame_period{};
    Shader* shader{};
    FT_Face font_file{};
    std::map<char32_t, lgo::chartex> character{};
    std::vector<lgo::text*> text{};
    State state{};
    std::vector<lgo::button*> button{};
    std::vector<Copy_holder<lgo::message_window_pv*>> message_window{};
    lgo::smat<float, 4, 4> view{};
    lgo::smat<float, 4, 4> projection{};
    Data();
};

class Home_data {
public:
    Home_data();
    ~Home_data();
};

class Environment_data {
public:
    Environment_data();
    ~Environment_data();
};

class World_data {
public:
    bool first_cursor{};
    float acceleration{};
    float max_speed{};
    float angle_of_view{};
    float friction{};
    float cursor_sensitivity{};
    lgo::svec<float, 3> ambient_color{};
    double select_length{};
    Surface* selected_surface{};
    lgo::svec<float, 3> selected_pos{};
    float last_cursor_pos_x{};
    float last_cursor_pos_y{};
    lgo::svec<float, 3> camera_pos{};
    lgo::svec<float, 3> camera_dir{};
    lgo::svec<float, 3> camera_up{};
    lgo::svec<float, 3> camera_speed{};
    float yaw{};
    float pitch{};
    lgo::arrs<lgo::arrs<Tile*, constant::tile_num>, constant::tile_num> tiles{};
    std::vector<std::vector<std::vector<Block*>>> blocks{};
    unsigned int pos_shininess{};
    unsigned int normal_specular_strength{};
    unsigned int color{};
    unsigned int parent{};
    World_data();
    ~World_data();
};

#endif