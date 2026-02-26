#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "text.h"
#include "data.h"
#include "debug_f.h"

Text::Text(std::u32string text, float height, glm::vec3 color_param,
	glm::mat4 transform_mat_param, Data_pv* data) {
	color = color_param;
	transform_mat = transform_mat_param;
	set_text(text, data);
	set_height(height, data);
	return;
}

void Text::render(unsigned int shader, Data_pv* data) {
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glUseProgram(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader, "transform_mat"),
		1, GL_FALSE, glm::value_ptr(transform_mat));
	glUniform3fv(glGetUniformLocation(shader, "color"), 1, glm::value_ptr(color));
	glUniform1i(glGetUniformLocation(shader, "tex"), 0);
	for (int i{ 0 }; i < text.size(); i++) {
		glBindVertexArray(VAO[i]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, data->character[text[i]].tex);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	glBindVertexArray(0);
	check_GL_error(data);
	return;
}

void Text::set_pos(glm::vec3 pos_param, Data_pv* data) {
	pos = pos_param;
	current_VBO(data);
	return;
}

void Text::set_horizontal_alignment(Alignment horizontal_alignment_param, Data_pv* data) {
	horizontal_alignment = horizontal_alignment_param;
	current_VBO(data);
	return;
}

void Text::set_vertical_alignment(Alignment vertical_alignment_param, Data_pv* data) {
	vertical_alignment = vertical_alignment_param;
	current_VBO(data);
	return;
}

void Text::set_height(float height, Data_pv* data) {
	vertical_scale = height / data->text_resolution;
	horizontal_scale = vertical_scale;
	current_VBO(data);
	return;
}

void Text::set_horizontal_dir(glm::vec3 horizontal_dir_param, Data_pv* data) {
	horizontal_dir = horizontal_dir_param;
	current_VBO(data);
	return;
}

void Text::set_vertical_dir(glm::vec3 vertical_dir_param, Data_pv* data) {
	vertical_dir = vertical_dir_param;
	current_VBO(data);
	return;
}

void Text::set_text(std::u32string text_param, Data_pv* data) {
	for (int i{ 0 }; i < text.size(); i++) {
		//...
	}
	for (int i{ static_cast<int>(text_param.size()) - 1 }; i >= static_cast<int>(text.size()); i--) {
		//...
	}
	text = text_param;
	for (int i{ 0 }; i < text.size(); i++) {
		if (data->character.count(text[i]) == 0) {
			if (FT_Load_Char(data->font_file, text[i], FT_LOAD_RENDER))
				handle_error(U"error: Failed to load char.\n", data);
			Character ch{};
			glGenTextures(1, &ch.tex);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ch.tex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, data->font_file->glyph->bitmap.width,
				data->font_file->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
				data->font_file->glyph->bitmap.buffer);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			ch.advance = data->font_file->glyph->advance.x;
			ch.width = data->font_file->glyph->bitmap.width;
			ch.height = data->font_file->glyph->bitmap.rows;
			ch.left_pos = data->font_file->glyph->bitmap_left;
			ch.top_pos = data->font_file->glyph->bitmap_top;
			ch.count = 0;
			data->character.insert(std::pair<char32_t, Character>{text[i], ch});
		}
		data->character[text[i]].count++;
	}
	for (int i{ static_cast<int>(VAO.size()) }; i < text.size(); i++) {
		unsigned int VAO_var{};
		glGenVertexArrays(1, &VAO_var);
		glBindVertexArray(VAO_var);
		unsigned int VBO_var{};
		glGenBuffers(1, &VBO_var);
		glBindBuffer(GL_ARRAY_BUFFER, VBO_var);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 6, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		VAO.push_back(VAO_var);
		VBO.push_back(VBO_var);
	}
	current_VBO(data);
	return;
}

void Text::current_VBO(Data_pv* data) {
	if (text.size() == 0 || glm::length(horizontal_dir) == 0 || glm::length(vertical_dir) == 0)
		return;
	float width{ 0.0f };
	int text_top{};
	int text_bottom{};
	for (int i{ 0 }; i < text.size(); i++) {
		width += (data->character[text[i]].advance >> 6) * horizontal_scale;
		int current_top{ data->character[text[i]].top_pos };
		int current_bottom{ current_top - data->character[text[i]].height };
		if (current_top > text_top)
			text_top = current_top;
		if (current_bottom < text_bottom)
			text_bottom = current_bottom;
	}
	glm::vec3 start_pos{ pos - horizontal_dir * width *
		(horizontal_alignment == Alignment::Small ? 0.0f :
			(horizontal_alignment == Alignment::Middle ? 0.5f : 1.0f))
		- vertical_dir * (vertical_alignment == Alignment::Small ? static_cast<float>(text_bottom) :
			(vertical_alignment == Alignment::Middle ? static_cast<float>(text_bottom + text_top) * 0.5f :
				static_cast<float>(text_top))) * vertical_scale };
	float offset{ 0.0f };
	for (int i{ 0 }; i < text.size(); i++) {
		glm::vec3 current_pos{ start_pos + offset * horizontal_dir };
		Character ch{ data->character[text[i]] };
		glm::vec3 vertex1{ current_pos + static_cast<float>(ch.left_pos) * horizontal_dir * horizontal_scale + static_cast<float>(ch.top_pos - ch.height) * vertical_dir * vertical_scale };
		glm::vec3 vertex2{ current_pos + static_cast<float>(ch.left_pos + ch.width) * horizontal_dir * horizontal_scale + static_cast<float>(ch.top_pos - ch.height) * vertical_dir * vertical_scale };
		glm::vec3 vertex3{ current_pos + static_cast<float>(ch.left_pos) * horizontal_dir * horizontal_scale + static_cast<float>(ch.top_pos) * vertical_dir * vertical_scale };
		glm::vec3 vertex4{ current_pos + static_cast<float>(ch.left_pos + ch.width) * horizontal_dir * horizontal_scale + static_cast<float>(ch.top_pos) * vertical_dir * vertical_scale };
		float vertex_data[6][5] = {
			{vertex1.x, vertex1.y, vertex1.z, 0.0f, 0.0f},
			{vertex2.x, vertex2.y, vertex2.z, 1.0f, 0.0f},
			{vertex3.x, vertex3.y, vertex3.z, 0.0f, 1.0f},
			{vertex3.x, vertex3.y, vertex3.z, 0.0f, 1.0f},
			{vertex2.x, vertex2.y, vertex2.z, 1.0f, 0.0f},
			{vertex4.x, vertex4.y, vertex4.z, 1.0f, 1.0f},
		};
		glBindBuffer(GL_ARRAY_BUFFER, VBO[i]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 6, vertex_data);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		offset += (ch.advance >> 6) * horizontal_scale;
	}
	return;
}