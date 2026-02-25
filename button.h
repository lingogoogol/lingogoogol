#ifndef LIBRARY_H
#define LIBRARY_H

#include <cstdint>
#include <fstream>
#include <functional>
#include <array>
#include <map>
#include <type_traits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "button_f.h"
#include "surface.h"
#include "debug_f.h"
#include "math.h"
#include "value_f.h"

enum class Button_state {
	Normal,
	Hovered,
	Clicked
};

class Button {
public:
	glm::mat4 transform_mat{};
	glm::vec3 normal_color{};
	glm::vec3 hovered_color{};
	glm::vec3 clicked_color{};
	State state{};
	std::function<void()> func{};
	Button();
	Button(float width_param, float height_param, glm::mat4 transform_mat_param,
		glm::vec3 normal_color_param, glm::vec3 hovered_color_param, glm::vec3 clicked_color_param,
		State state_param, std::function<void(void)> func_param = []() {});
	void set_pos(glm::vec3 pos_param);
	void print();
	bool update_state_hover(float x, float y);
	bool update_state_click(int action);
private:
	glm::vec3 pos{ 0.0f,0.0f,0.0f };
	Alignment horizontal_alignment{ Alignment::Middle };
	Alignment vertical_alignment{ Alignment::Middle };
	float width{};
	float height{};
	float left{};
	float right{};
	float bottom{};
	float top{};
	unsigned int VAO{};
	unsigned int VBO{};
	Button_state button_state{ Button_state::Normal };
	void allocate_VAO();
	void current_VBO();
};

#endif