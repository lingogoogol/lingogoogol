#ifndef LGO_DATA_D_H
#define LGO_DATA_D_H

#include <vector>
#include <map>

#include "f.h"
#include "../lib/glad.h"
#include "../lib/glfw.h"
#include "../lib/freetype.h"
#include "../holder_f.h"
#include "../graphic/f.h"
#include "../text/f.h"
#include "../file/.h"

namespace lgo {
	namespace impl {
		class data {
		public:
			int text_resolution{};
			file<char>* logfile{};
			std::vector<Copy_holder<message_window_pv*>>* message_windows{};
			FT_Face* font_file{};
			std::map<char32_t, chartex>* chars{};
			GLFWwindow** main_window{};
		};
	}
}

#endif