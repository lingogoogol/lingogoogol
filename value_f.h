#ifndef VALUE_F_H
#define VALUE_F_H

#include <map>
#include <vector>

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "button_f.h"
#include "text_f.h"
#include "window_f.h"
#include "holder_f.h"
#include "version.h"

enum class State;

struct Character;
struct Shader;
struct Setting;

namespace constant {
    inline const Version current_version{ 0,0,0,0,1 };
    inline const Version last_necessary_version{ 0,0,0,0,1 };
    inline constexpr unsigned int OpenGL_version_major{ 4 };
    inline constexpr unsigned int OpenGL_version_minor{ 6 };
    inline constexpr unsigned int message_window_width{ 800 };
    inline constexpr unsigned int message_window_height{ 300 };
    inline const float message_text_height{ 50.0f };
    inline const glm::vec3 message_text_color{ 0.0f,0.0f,0.0f };
    inline constexpr unsigned int error_message_max_size{ 1024 };
    inline const float pi_floor{ 3.141f };
    inline const float pi_ceil{ 3.142f };
    inline const std::string GLSL_version{ "460" };
    inline const std::string text_vertex_shader{ "#version " + GLSL_version + " core\n"
        + "layout (location = 0) in vec3 vertex;\n"
        + "layout (location = 1) in vec2 texcoord;\n"
        + "out vec2 texcoord_frag;\n"
        + "uniform mat4 transform_mat;\n"
        + "void main() {\n"
        + "    gl_Position = transform_mat * vec4(vertex, 1.0);\n"
        + "    texcoord_frag = texcoord;\n"
        + "}\n" };
    inline const std::string text_fragment_shader{ "#version " + GLSL_version + " core\n"
        + "in vec2 texcoord_frag;\n"
        + "out vec4 color_out;\n"
        + "uniform vec3 color;\n"
        + "uniform sampler2D tex;\n"
        + "void main() {\n"
        + "    float alpha = texture(tex, texcoord_frag).r;\n"
        + "    if(alpha == 0.0)\n"
        + "        discard;\n"
        + "    color_out = vec4(color, 1.0);\n"
        + "}\n" };
    inline const std::string button_vertex_shader{ "#version " + GLSL_version + " core\n"
        + "layout (location = 0) in vec3 pos;\n"
        + "uniform mat4 transform_mat;\n"
        + "void main() {\n"
        + "    gl_Position = transform_mat * vec4(pos, 1.0);\n"
        + "}\n" };
    inline const std::string button_fragment_shader{ "#version " + GLSL_version + " core\n"
        + "out vec4 color_out;\n"
        + "uniform vec3 color;\n"
        + "void main() {\n"
        + "    color_out = vec4(color, 1.0);\n"
        + "}\n" };
    inline const std::string cell_surface_vertex_shader{ "#version " + GLSL_version + " core\n"
        + "layout (location = 0) in vec3 pos;\n"
        + "layout (location = 1) in vec2 texcoord;\n"
        + "layout (location = 2) in vec3 normal;\n"
        + "out vec3 pos_frag;\n"
        + "out vec2 texcoord_frag;\n"
        + "out vec3 normal_frag;\n"
        + "uniform mat4 view;\n"
        + "uniform mat4 projection;\n"
        + "void main() {\n"
        + "    gl_Position = projection * view * vec4(pos, 1.0);\n"
        + "    pos_frag = pos;\n"
        + "    texcoord_frag = texcoord;\n"
        + "    normal_frag = normal;\n"
        + "}\n" };
    inline const std::string object_surface_vertex_shader{ "#version " + GLSL_version + " core\n"
        + "layout (location = 0) in vec3 pos;\n"
        + "layout (location = 1) in vec2 texcoord;\n"
        + "layout (location = 2) in vec3 normal;\n"
        + "out vec3 pos_frag;\n"
        + "out vec2 texcoord_frag;\n"
        + "out vec3 normal_frag;\n"
        + "uniform mat4 model;\n"
        + "uniform mat4 view;\n"
        + "uniform mat4 projection;\n"
        + "void main() {\n"
        + "    gl_Position = projection * view * model * vec4(pos, 1.0);\n"
        + "    pos_frag = pos;\n"
        + "    texcoord_frag = texcoord;\n"
        + "    normal_frag = normal;\n"
        + "}\n" };
    inline const std::string surface_fragment_shader{ "#version " + GLSL_version + " core\n"
        + "struct Point_light {\n"
        + "    vec3 pos;\n"
        + "    vec3 color;\n"
        + "};\n"
        + "in vec3 pos_frag;\n"
        + "in vec2 texcoord_frag;\n"
        + "in vec3 normal_frag;\n"
        + "out vec4 color;\n"
        + "uniform sampler2D tex;\n"
        + "uniform vec3 camera_pos;\n"
        + "uniform Point_light point_light;\n"
        + "uniform vec3 ambient_color;\n"
        + "uniform float shininess;\n"
        + "uniform float specular_strength;\n"
        + "void main() {\n"
        + "    vec3 light_dir = normalize(point_light.pos - pos_frag);\n"
        + "    vec3 camera_dir = normalize(camera_pos - pos_frag);\n"
        + "    vec3 diffuse_color = point_light.color * max(dot(normal_frag, light_dir), 0.0);\n"
        + "    vec3 specular_color = point_light.color * pow(max(dot(reflect(-light_dir, normal_frag), camera_dir), 0.0), shininess) * specular_strength;\n"
        + "    color = vec4((ambient_color + diffuse_color + specular_color), 1.0) * texture(tex, texcoord_frag);\n"
        + "}\n" };
}

namespace object {
    extern GLFWwindow* main_window;
    extern std::ofstream logfile;
    extern float current_time;
    extern float last_time;
    extern float frame_period;
    extern bool first_cursor;
    extern float last_cursor_pos_x;
    extern float last_cursor_pos_y;
    extern glm::vec3 camera_pos;
    extern glm::vec3 camera_dir;
    extern glm::vec3 camera_up;
    extern glm::vec3 camera_speed;
    extern float yaw;
    extern float pitch;
    extern Shader shader;
    extern Setting setting;
    extern FT_Face font_file;
    extern std::map<char32_t, Character> character;
    extern std::vector<Text*> text;
    extern State state;
    extern std::vector<Button*> button;
    extern std::vector<Copy_holder<Message_window_pv*>> message_window;
}

void init_setting();

#endif