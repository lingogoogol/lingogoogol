#ifndef TEXT_H
#define TEXT_H

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "data_f.h"
#include "text_f.h"
#include "math.h"

struct Character {
	int advance{};
	int left_pos{};
	int top_pos{};
	int width{};
	int height{};
	unsigned int tex{};
	unsigned int count{};
};

class Text {
public:
	glm::vec3 color{};
	glm::mat4 transform_mat{};
	Text(std::u32string text, float height, glm::vec3 color_param,
		glm::mat4 transform_mat_param, Data_pv* data);
	void set_pos(glm::vec3 pos_param, Data_pv* data);
	void set_horizontal_alignment(Alignment horizontal_alignment_param, Data_pv* data);
	void set_vertical_alignment(Alignment vertical_alignment_param, Data_pv* data);
	void set_height(float height, Data_pv* data);
	void set_horizontal_dir(glm::vec3 horizontal_dir_param, Data_pv* data);
	void set_vertical_dir(glm::vec3 vertical_dir_param, Data_pv* data);
	void set_text(std::u32string text_param, Data_pv* data);
	void render(unsigned int shader, Data_pv* data);
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
	void current_VBO(Data_pv* data);
};

#endif