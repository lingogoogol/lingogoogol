#ifndef SHADER_F_H
#define SHADER_F_H

#include <string>

struct Shader;

unsigned int create_shader(std::string vertex, std::string fragment, Data_pv* data);

#endif