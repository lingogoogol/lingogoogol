#ifndef INIT_H
#define INIT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "init_f.h"

enum class Data_pointer {
	Version,
	User_name = Version + 8
};

#endif