#ifndef CONST_F_H
#define CONST_F_H

#include <string>

#include <glm/glm.hpp>

#include "lib.h"

namespace constant {
    inline const Version current_version{ 0,0,0,1,5 };
    inline const Version last_necessary_version{ 0,0,0,0,1 };
    inline constexpr int OpenGL_version_major{ 4 };
    inline constexpr int OpenGL_version_minor{ 6 };
    inline constexpr int main_window_width{ 800 };
    inline constexpr int main_window_height{ 600 };
    inline constexpr int message_window_width{ 800 };
    inline constexpr int message_window_height{ 300 };
    inline const float message_text_height{ 50.0f };
    inline const glm::vec3 message_text_color{ 0.0f,0.0f,0.0f };
    inline constexpr int surface_tex_precision{ 2 };
    inline constexpr int tile_num{ 1 };
    inline constexpr int cell_num{ 16 };
    inline constexpr int block_num{ 1 };
    inline const float pi_floor{ 3.141f };
    inline const float pi_ceil{ 3.142f };
    inline const std::string GLSL_version{ "460" };
    inline const std::string button_vertex_shader{
        "#version " + GLSL_version + " core\n"
        "layout (location = 0) in vec3 pos;\n"
        "uniform mat4 transform_mat;\n"
        "void main() {\n"
        "    gl_Position = transform_mat * vec4(pos, 1.0);\n"
        "}\n"
    };
    inline const std::string button_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "out vec4 color_out;\n"
        "uniform vec3 color;\n"
        "void main() {\n"
        "    color_out = vec4(color, 1.0);\n"
        "}\n"
    };
    inline const std::string transparent_cell_vertex_shader{
        "#version " + GLSL_version + " core\n"
        "layout (location = 0) in vec3 pos;\n"
        "layout (location = 1) in vec2 texcoord;\n"
        "out vec3 pos_frag;\n"
        "out vec2 texcoord_frag;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * view * vec4(pos, 1.0);\n"
        "    pos_frag = pos;\n"
        "    texcoord_frag = texcoord;\n"
        "}\n"
    };
    inline const std::string transparent_cell_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "in vec3 pos_frag;\n"
        "in vec2 texcoord_frag;\n"
        "layout (location = 0) out vec4 pos_shininess_out;\n"
        "layout (location = 1) out vec4 normal_specular_strength_out;\n"
        "layout (location = 2) out vec4 color_out;\n"
        "layout (location = 3) out ivec4 parent;\n"
        "uniform sampler2D normal;\n"
        "uniform sampler2D color;\n"
        "uniform sampler2D shininess;\n"
        "uniform sampler2D specular_strength;\n"
        "uniform ivec3 parent_cell;\n"
        "void main() {\n"
        "    pos_shininess_out = vec4(pos_frag, texture(shininess, texcoord_frag).r);\n"
        "    normal_specular_strength_out = vec4(texture(normal, texcoord_frag).rgb, texture(specular_strength, texcoord_frag).r);\n"
        "    color_out = texture(color, texcoord_frag);\n"
        "    parent = ivec4(0, parent_cell);\n"
        "}\n"
    };
    /*inline const std::string transparent_object_vertex_shader{
        "#version " + GLSL_version + " core\n"
    };
    inline const std::string transparent_object_fragment_shader{
        "#version " + GLSL_version + " core\n"
    };*/
    inline const std::string tile_vertex_shader{
        "#version " + GLSL_version + " core\n"
        "layout (location = 0) in vec2 screen_pos;\n"
        "layout (location = 1) in vec2 texcoord;\n"
        "out vec2 texcoord_frag;\n"
        "void main() {\n"
        "    gl_Position = vec4(screen_pos, 0.0, 1.0);\n"
        "    texcoord_frag = texcoord;\n"
        "}\n"
    };
    inline const std::string tile_ambient_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "in vec2 texcoord_frag;\n"
        "out vec4 color_out;\n"
        "uniform sampler2D color;\n"
        "uniform vec3 ambient_color;\n"
        "void main() {\n"
        "    color_out = vec4(ambient_color, 1.0) * texture(color, texcoord_frag);\n"
        "}\n"
    };
    /*inline const std::string tile_dir_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "struct Dir_light {\n"
        "    vec3 dir;\n"
        "    vec3 color;\n"
        "};\n"
        "in vec2 texcoord_frag;\n"
        "out vec4 color_out;\n"
        "uniform sampler2D pos;\n"
        "uniform sampler2D color;\n"
        "uniform sampler2D normal;\n"
        "uniform sampler2D shininess_specular_strength;\n"
        "uniform Dir_light light;\n"
        "uniform vec3 camera_pos;\n"
        "uniform vec3 camera_dir;\n"
        "void main() {\n"
        "    vec3 light_dir = normalize(point_light.pos - pos_frag);\n"
        "    vec3 camera_dir = normalize(camera_pos - pos_frag);\n"
        "    vec3 diffuse_color = point_light.color * max(dot(normal_frag, light_dir), 0.0);\n"
        "    vec3 specular_color = point_light.color * pow(max(dot(reflect(-light_dir, normal_frag), camera_dir), 0.0), shininess) * specular_strength;\n"
        "    color = vec4((ambient_color + diffuse_color + specular_color), 1.0) * texture(tex, texcoord_frag);\n"
        "}\n"
    };*/
    inline const std::string tile_point_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "struct Point_light {\n"
        "    vec3 pos;\n"
        "    vec3 color;\n"
        "};\n"
        "in vec2 texcoord_frag;\n"
        "out vec4 color_out;\n"
        "uniform sampler2D pos_shininess;\n"
        "uniform sampler2D normal_specular_strength;\n"
        "uniform sampler2D color;\n"
        "uniform Point_light light;\n"
        "uniform vec3 camera_pos;\n"
        "uniform vec3 camera_dir;\n"
        "void main() {\n"
        "    vec3 pos = texture(pos_shininess, texcoord_frag).rgb;\n"
        "    vec3 normal = texture(normal_specular_strength, texcoord_frag).rgb;\n"
        "    float shininess = texture(pos_shininess, texcoord_frag).a;\n"
        "    float specular_strength = texture(normal_specular_strength, texcoord_frag).a;\n"
        "    vec3 light_dir = normalize(light.pos - pos);\n"
        "    vec3 camera_dir = normalize(camera_pos - pos);\n"
        "    vec3 diffuse_color = light.color * max(dot(normal, light_dir), 0.0);\n"
        "    vec3 specular_color = light.color * pow(max(dot(reflect(-light_dir, normal),\n"
        "        camera_dir), 0.0), shininess) * specular_strength;\n"
        "    color_out = vec4(diffuse_color + specular_color, 1.0) * texture(color, texcoord_frag);\n"
        "}\n"
    };
    /*inline const std::string tile_spot_fragment_shader{
        "#version " + GLSL_version + " core\n"
        "struct Dir_light {\n"
        "    vec3 dir;\n"
        "    vec3 color;\n"
        "};\n"
        "struct Point_light {\n"
        "    vec3 pos;\n"
        "    vec3 color;\n"
        "};\n"
        "struct Spotlight {\n"
        "    vec3 pos;\n"
        "    vec3 dir;\n"
        "    vec3 color;\n"
        "    float phi;\n"
        "    float theta;\n"
        "};\n"
        "in vec3 screen_pos_frag;\n"
        "in vec2 texcoord_frag;\n"
        "out vec4 color_out;\n"
        "uniform sampler2D pos;\n"
        "uniform sampler2D color;\n"
        "uniform sampler2D normal_shininess;\n"
        "uniform sampler2D ambient_color_specular_strength;\n"
        "uniform Dir_light dir_light;\n"
        "uniform Point_light[16] point_light;\n"
        "uniform Spotlight[16] spotlight;\n"
        "uniform vec3 camera_pos;\n"
        "uniform vec3 camera_dir;\n"
        "void main() {\n"
        "    vec3 light_dir = normalize(point_light.pos - pos_frag);\n"
        "    vec3 camera_dir = normalize(camera_pos - pos_frag);\n"
        "    vec3 diffuse_color = point_light.color * max(dot(normal_frag, light_dir), 0.0);\n"
        "    vec3 specular_color = point_light.color * pow(max(dot(reflect(-light_dir, normal_frag), camera_dir), 0.0), shininess) * specular_strength;\n"
        "    color = vec4((ambient_color + diffuse_color + specular_color), 1.0) * texture(tex, texcoord_frag);\n"
        "}\n"
    };
    inline const std::string opaque_cell_vertex_shader{};
    inline const std::string opaque_cell_fragment_shader{};
    inline const std::string opaque_object_vertex_shader{};
    inline const std::string opaque_object_fragment_shader{};*/
}

#endif