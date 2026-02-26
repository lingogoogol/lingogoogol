#ifndef DATA_H
#define DATA_H

#include <string>
#include <fstream>
#include <map>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "const_f.h"
#include "lib.h"
#include "tile_f.h"
#include "block_f.h"

enum class State {
    Home,
    Environment,
    World
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

struct Data_pv {
public:
    int text_resolution{};
    GLFWwindow* main_window{};
    std::ofstream* logfile{};
    float current_time{};
    float last_time{};
    float frame_period{};
    Shader* shader{};
    FT_Face font_file{};
    std::map<char32_t, Character> character{};
    std::vector<Text*> text{};
    State state{};
    std::vector<Button*> button{};
    std::vector<Copy_holder<Message_window_pv*>> message_window{};
    glm::mat4 view{};
    glm::mat4 projection{};
    Data_pv();
    ~Data_pv();
};

struct Home_data :public Data_pv {
public:
    void setup();
};

struct Environment_data :public Data_pv {
public:
    void setup();
};

struct World_data :public Data_pv {
public:
    bool first_cursor{};
    float acceleration{};
    float max_speed{};
    float angle_of_view{};
    float friction{};
    float cursor_sensitivity{};
    glm::vec3 ambient_color{};
    float last_cursor_pos_x{};
    float last_cursor_pos_y{};
    glm::vec3 camera_pos{};
    glm::vec3 camera_dir{};
    glm::vec3 camera_up{};
    glm::vec3 camera_speed{};
    float yaw{};
    float pitch{};
    std::array<std::array<Tile*, constant::tile_num>, constant::tile_num> tiles{};
    std::vector<std::vector<std::vector<Copy_holder<Block*>>>> blocks{};
    unsigned int pos_shininess{};
    unsigned int normal_specular_strength{};
    unsigned int color{};
    unsigned int parent{};
    World_data() = default;
    void setup();
};

#endif