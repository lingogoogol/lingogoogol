#include <fstream>
#include <map>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "value.h"
#include "holder.h"
#include "tile.h"
#include "text.h"
#include "block.h"

namespace object {
    GLFWwindow* main_window{};
    std::ofstream logfile{};
    float current_time{};
    float last_time{};
    float frame_period{};
    bool first_cursor{ true };
    float last_cursor_pos_x{};
    float last_cursor_pos_y{};
    glm::vec3 camera_pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 camera_dir{ 0.0f, 0.0f, 1.0f };
    glm::vec3 camera_up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 camera_speed{};
    float yaw{ constant::pi_floor / 2.0f };
    float pitch{ 0.0f };
    Shader shader{};
    Setting setting{};
    FT_Face font_file{};
    std::map<char32_t, Character> character{};
    std::vector<Text*> text{};
    State state{};
    std::vector<Button*> button{};
    std::vector<Copy_holder<Message_window_pv*>> message_window{};
    std::array<std::array<Tile*, constant::tile_num_horizontal>, constant::tile_num_vertical> tiles{};
    std::vector<std::vector<std::vector<Copy_holder<Block*>>>> blocks{};
    glm::mat4 view{};
    glm::mat4 projection{};
}