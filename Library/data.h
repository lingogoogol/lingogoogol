#ifndef LIBRARY_DATA_H
#define LIBRARY_DATA_H

#include <fstream>
#include <vector>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "data_f.h"
#include "holder_f.h"
#include "graphic_f.h"
#include "text_f.h"

namespace implement {
	struct Data {
		int text_resolution{};
		std::ofstream* logfile{};
		std::vector<Copy_holder<Message_window_pv*>>* message_windows{};
		FT_Face* font_file{};
		std::map<char32_t, Character>* chars{};
		GLFWwindow** main_window{};
	};
}

#endif