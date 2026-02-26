#ifndef TEXT_H
#define TEXT_H

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "text_f.h"
#include "math.h"

class Text {
public:
	glm::vec3 color{};
	glm::mat4 transform_mat{};
	Text();
	Text(std::u32string text, float height, glm::vec3 color_param, glm::mat4 transform_mat_param);
	void set_pos(glm::vec3 pos_param);
	void set_horizontal_alignment(Alignment horizontal_alignment_param);
	void set_vertical_alignment(Alignment vertical_alignment_param);
	void set_height(float height);
	void set_horizontal_dir(glm::vec3 horizontal_dir_param);
	void set_vertical_dir(glm::vec3 vertical_dir_param);
	void set_text(std::u32string text_param);
	void print();
private:
	glm::vec3 pos{ 0.0f,0.0f,0.0f };
	Alignment horizontal_alignment{ Alignment::Middle };
	Alignment vertical_alignment{ Alignment::Middle };
	float horizontal_scale{};
	float vertical_scale{};
	glm::vec3 horizontal_dir{ 1.0f,0.0f,0.0f };
	glm::vec3 vertical_dir{ 0.0f,1.0f,0.0f };
	std::u32string text{};
	std::vector<unsigned int> VAO{};
	std::vector<unsigned int> VBO{};
	void current_VBO();
};

#endif