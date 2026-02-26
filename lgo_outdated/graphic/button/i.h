#ifndef LGO_BUTTON_I_H
#define LGO_BUTTON_I_H

#include "d.h"
#include "../graphic/d.h"
#include "../lib/glad.h"

namespace lgo {
	inline button::button(float width_param, float height_param,
		smat<float, 4, 4> transform_mat_param,
		svec<float, 3> normal_color_param, svec<float, 3> hovered_color_param,
		svec<float, 3> clicked_color_param, std::function<void(button&)> func_param) :
		m_width{ width_param }, m_height{ height_param },
		m_transform_mat{ transform_mat_param }, m_normal_color{ normal_color_param },
		m_hovered_color{ hovered_color_param }, m_clicked_color{ clicked_color_param },
		m_func{ func_param } {
		allocate_VAO();
		current_VBO();
	}

	inline void button::set_pos(svec<float, 3> pos_param) {
		m_pos = pos_param;
		current_VBO();
	}

	inline void button::render(unsigned int shader) {
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glUseProgram(shader);
		glUniform3fv(glGetUniformLocation(shader, "color"), 1,
			(m_state == button_state::normal ? m_normal_color :
				(m_state == button_state::hovered ? m_hovered_color : m_clicked_color)).data());
		float* transform_mat_pointer{ make_gl_matrix(m_transform_mat) };
		glUniformMatrix4fv(glGetUniformLocation(shader, "transform_mat"),
			1, GL_FALSE, transform_mat_pointer);
		free_gl_matrix(transform_mat_pointer);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	inline bool button::update_state_hover(float x, float y, GLFWwindow* window) {
		if (x >= m_left && x < m_right && y >= m_bottom && y < m_top)
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
				m_state = button_state::clicked;
			else
				m_state = button_state::hovered;
		else
			m_state = button_state::normal;
		return m_state != button_state::normal;
	}

	inline bool button::update_state_click(int action) {
		if (m_state != button_state::normal) {
			if (action == GLFW_PRESS) {
				m_state = button_state::clicked;
			}
			else if (action == GLFW_RELEASE) {
				m_func(*this);
			}
		}
		return m_state != button_state::normal;
	}

	inline void button::allocate_VAO() {
		glGenVertexArrays(1, &m_vao);
		glBindVertexArray(m_vao);
		glGenBuffers(1, &m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 6, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, rc<void*>(0));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	inline void button::current_VBO() {
		m_left = m_pos[0] - m_width * (m_halign == dir::small ? 0.0f :
			(m_halign == dir::middle ? 0.5f : 1.0f));
		m_right = m_left + m_width;
		m_bottom = m_pos[1] - m_height * (m_valign == dir::small ? 0.0f :
			(m_valign == dir::middle ? 0.5f : 1.0f));
		m_top = m_bottom + m_height;
		float vertices[6][3]{
			{ m_right, m_top, m_pos[2] },
			{ m_right, m_bottom, m_pos[2] },
			{ m_left, m_top, m_pos[2] },
			{ m_left, m_top, m_pos[2] },
			{ m_right, m_bottom, m_pos[2] },
			{ m_left, m_bottom, m_pos[2] }
		};
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 6, vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

#endif