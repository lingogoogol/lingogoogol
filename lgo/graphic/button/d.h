#ifndef LGO_BUTTON_D_H
#define LGO_BUTTON_D_H

#include <cstdint>
#include <functional>
#include <map>
#include <type_traits>

#include "f.h"
#include "../lib/glad.h"
#include "../lib/glfw.h"
#include "../math/d.h"
#include "../vec/.h"
#include "../mat/.h"

namespace lgo {
	enum class button_state {
		normal,
		hovered,
		clicked
	};

	class button {
	public:
		button(float width_param, float height_param, smat<float, 4, 4> transform_mat_param,
			svec<float, 3> normal_color_param, svec<float, 3> hovered_color_param,
			svec<float, 3> clicked_color_param, std::function<void(button&)> func_param);
		void set_pos(svec<float, 3> pos_param);
		void render(unsigned int shader);
		bool update_state_hover(float x, float y, GLFWwindow* window);
		bool update_state_click(int action);
	private:
		svec<float, 3> m_pos{ 0.0f,0.0f,0.0f };
		dir m_halign{ dir::middle };
		dir m_valign{ dir::middle };
		float m_width{};
		float m_height{};
		float m_left{};
		float m_right{};
		float m_bottom{};
		float m_top{};
		smat<float, 4, 4> m_transform_mat{};
		svec<float, 3> m_normal_color{};
		svec<float, 3> m_hovered_color{};
		svec<float, 3> m_clicked_color{};
		std::function<void(button&)> m_func{};
		unsigned int m_vao{};
		unsigned int m_vbo{};
		button_state m_state{ button_state::normal };
		void allocate_VAO();
		void current_VBO();
	};
}

#endif