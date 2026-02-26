#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "value.h"
#include "button.h"

Button::Button() {
	allocate_VAO();
	return;
}

Button::Button(float width_param, float height_param, glm::mat4 transform_mat_param,
	glm::vec3 normal_color_param, glm::vec3 hovered_color_param, glm::vec3 clicked_color_param,
	State state_param, std::function<void()> func_param) :
	width{ width_param }, height{ height_param }, transform_mat{ transform_mat_param },
	normal_color{ normal_color_param }, hovered_color{ hovered_color_param }, clicked_color{ clicked_color_param },
	state{ state_param }, func{ func_param }{
	allocate_VAO();
	current_VBO();
	return;
}

void Button::set_pos(glm::vec3 pos_param) {
	pos = pos_param;
	current_VBO();
	return;
}

void Button::print() {
	glUseProgram(object::shader.button);
	glUniform3fv(glGetUniformLocation(object::shader.button, "color"), 1, glm::value_ptr(button_state == Button_state::Normal ? normal_color :
		(button_state == Button_state::Hovered ? hovered_color : clicked_color)));
	glUniformMatrix4fv(glGetUniformLocation(object::shader.button, "transform_mat"), 1, GL_FALSE, glm::value_ptr(transform_mat));
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	return;
}

bool Button::update_state_hover(float x, float y) {
	if (x >= left && x < right && y >= bottom && y < top)
		if (glfwGetMouseButton(object::main_window, GLFW_MOUSE_BUTTON_LEFT))
			button_state = Button_state::Clicked;
		else
			button_state = Button_state::Hovered;
	else
		button_state = Button_state::Normal;
	return button_state != Button_state::Normal;
}

bool Button::update_state_click(int action) {
	if (button_state != Button_state::Normal)
		if (action == GLFW_PRESS)
			button_state = Button_state::Clicked;
		else if (action == GLFW_RELEASE) {
			object::state = state;
			func();
		}
	return button_state != Button_state::Normal;
}

void Button::allocate_VAO() {
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 6, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Button::current_VBO() {
	left = pos.x - width * (horizontal_alignment == Alignment::Small ? 0.0f :
		(horizontal_alignment == Alignment::Middle ? 0.5f : 1.0f));
	right = left + width;
	bottom = pos.y - height * (vertical_alignment == Alignment::Small ? 0.0f :
		(vertical_alignment == Alignment::Middle ? 0.5f : 1.0f));
	top = bottom + height;
	float vertices[6][3]{
		{left,bottom,pos.z},
		{right,bottom,pos.z},
		{left,top,pos.z},
		{left,top,pos.z},
		{right,bottom,pos.z},
		{right,top,pos.z},
	};
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * 6, vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return;
}