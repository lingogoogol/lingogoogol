#ifndef LIBRARY_CONST_F_H
#define LIBRARY_CONST_F_H

#include <string>

#ifndef GLSL_VERSION
#define GLSL_VERSION "460"
#endif
#ifndef OPENGL_VERSION_MAJOR
#define OPENGL_VERSION_MAJOR 4
#endif
#ifndef OPENGL_VERSION_MINOR
#define OPENGL_VERSION_MINOR 6
#endif

namespace implement {
    inline constexpr int error_message_max_size{ 1024 };
    inline const std::string text_vertex_shader{
        "#version " GLSL_VERSION " core\n"
        "layout (location = 0) in vec3 vertex;\n"
        "layout (location = 1) in vec2 texcoord;\n"
        "out vec2 texcoord_frag;\n"
        "uniform mat4 transform_mat;\n"
        "void main() {\n"
        "    gl_Position = transform_mat * vec4(vertex, 1.0);\n"
        "    texcoord_frag = texcoord;\n"
        "}\n"
    };
    inline const std::string text_fragment_shader{
        "#version " GLSL_VERSION " core\n"
        "in vec2 texcoord_frag;\n"
        "out vec4 color_out;\n"
        "uniform vec3 color;\n"
        "uniform sampler2D tex;\n"
        "void main() {\n"
        "    float alpha = texture(tex, texcoord_frag).r;\n"
        "    if(alpha == 0.0)\n"
        "        discard;\n"
        "    color_out = vec4(color, 1.0);\n"
        "}\n"
    };
}

#endif