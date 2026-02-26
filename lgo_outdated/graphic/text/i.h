#ifndef LGO_TEXT_I_H
#define LGO_TEXT_I_H

#include "d.h"
#include "../lib/glad.h"
#include "../lib/freetype.h"
#include "../debug/f.h"
#include "../data/d.h"

namespace lgo {
	inline text::text(std::u32string text, float height, svec<float, 3> color_param,
		smat<float, 4, 4> transform_mat_param) {
		m_color = color_param;
		m_transform_mat = transform_mat_param;
		set_text(text);
		set_height(height);
	}

	inline void text::render(unsigned int shader) {
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glUseProgram(shader);
		float* transform_mat_pointer{ make_gl_matrix(m_transform_mat) };
		glUniformMatrix4fv(glGetUniformLocation(shader, "transform_mat"),
			1, GL_FALSE, transform_mat_pointer);
		free_gl_matrix(transform_mat_pointer);
		glUniform3fv(glGetUniformLocation(shader, "color"), 1, m_color.data());
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		for (int i{ 0 }; i < sc<intf64>(m_text.size()); ++i) {
			glBindVertexArray(m_vao[i]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, (*impl::get_data()->chars)[m_text[i]].tex);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glBindVertexArray(0);
	}

	inline void text::set_pos(svec<float, 3> pos_param) {
		pos = pos_param;
		current_vbo();
	}

	inline void text::set_horizontal_alignment(dir horizontal_alignment_param) {
		m_halign = horizontal_alignment_param;
		current_vbo();
	}

	inline void text::set_vertical_alignment(dir vertical_alignment_param) {
		m_valign = vertical_alignment_param;
		current_vbo();
	}

	inline void text::set_height(float height) {
		m_vscale = height / impl::get_data()->text_resolution;
		m_hscale = m_vscale;
		current_vbo();
	}

	inline void text::set_horizontal_dir(svec<float, 3> horizontal_dir_param) {
		m_hdir = horizontal_dir_param;
		current_vbo();
	}

	inline void text::set_vertical_dir(svec<float, 3> vertical_dir_param) {
		m_vdir = vertical_dir_param;
		current_vbo();
	}

	inline void text::set_text(std::u32string text_param) {
		for (int i{ 0 }; i < sc<intf64>(m_text.size()); ++i) {
			//...
		}
		for (int i{ sc<int>(text_param.size()) - 1 }; i >= sc<int>(m_text.size()); --i) {
			//...
		}
		m_text = text_param;
		for (int i{ 0 }; i < sc<intf64>(m_text.size()); ++i) {
			if (impl::get_data()->chars->count(m_text[i]) == 0) {
				if (FT_Load_Char(*impl::get_data()->font_file, m_text[i], FT_LOAD_RENDER))
					handle_error(U"error: Failed to load char.\n");
				chartex ch{};
				glGenTextures(1, &ch.tex);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, ch.tex);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
					(*impl::get_data()->font_file)->glyph->bitmap.width,
					(*impl::get_data()->font_file)->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
					(*impl::get_data()->font_file)->glyph->bitmap.buffer);
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glBindTexture(GL_TEXTURE_2D, 0);
				ch.advance = (*impl::get_data()->font_file)->glyph->advance.x;
				ch.width = (*impl::get_data()->font_file)->glyph->bitmap.width;
				ch.height = (*impl::get_data()->font_file)->glyph->bitmap.rows;
				ch.left_pos = (*impl::get_data()->font_file)->glyph->bitmap_left;
				ch.top_pos = (*impl::get_data()->font_file)->glyph->bitmap_top;
				ch.count = 0;
				impl::get_data()->chars->insert(std::pair<char32_t, chartex>{ m_text[i], ch });
			}
			(*impl::get_data()->chars)[m_text[i]].count++;
		}
		for (int i{ sc<int>(m_vao.size()) }; i < sc<intf64>(m_text.size()); ++i) {
			unsigned int VAO_var{};
			glGenVertexArrays(1, &VAO_var);
			glBindVertexArray(VAO_var);
			unsigned int VBO_var{};
			glGenBuffers(1, &VBO_var);
			glBindBuffer(GL_ARRAY_BUFFER, VBO_var);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 6, nullptr, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, rc<void*>(0));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, rc<void*>(sizeof(float) * 3));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			m_vao.push_back(VAO_var);
			m_vbo.push_back(VBO_var);
		}
		current_vbo();
	}

	inline void text::current_vbo() {
		if (m_text.size() == 0 || len(m_hdir) == 0 || len(m_vdir) == 0)
			return;
		float width{ 0.0f };
		int text_top{};
		int text_bottom{};
		for (int i{ 0 }; i < sc<intf64>(m_text.size()); ++i) {
			width += ((*impl::get_data()->chars)[m_text[i]].advance >> 6) * m_hscale;
			int top{ (*impl::get_data()->chars)[m_text[i]].top_pos };
			int bottom{ top - (*impl::get_data()->chars)[m_text[i]].height };
			if (top > text_top)
				text_top = top;
			if (bottom < text_bottom)
				text_bottom = bottom;
		}
		svec<float, 3> start_pos{ pos - m_hdir * width *
			(m_halign == dir::small ? 0.0f : (m_halign == dir::middle ?
			0.5f : 1.0f)) - m_vdir * (m_valign == dir::small ?
			sc<float>(text_bottom) : (m_valign == dir::middle ?
			sc<float>(text_bottom + text_top) * 0.5f : sc<float>(text_top))) * m_vscale };
		float offset{ 0.0f };
		for (int i{ 0 }; i < sc<intf64>(m_text.size()); ++i) {
			svec<float, 3> current_pos{ start_pos + offset * m_hdir };
			chartex ch{ (*impl::get_data()->chars)[m_text[i]] };
			svec<float, 3> vertex1{ current_pos +
				sc<float>(ch.left_pos + ch.width) * m_hdir * m_hscale +
				sc<float>(ch.top_pos) * m_vdir * m_vscale };
			svec<float, 3> vertex2{ current_pos +
				sc<float>(ch.left_pos + ch.width) * m_hdir * m_hscale +
				sc<float>(ch.top_pos - ch.height) * m_vdir * m_vscale };
			svec<float, 3> vertex3{ current_pos +
				sc<float>(ch.left_pos) * m_hdir * m_hscale +
				sc<float>(ch.top_pos) * m_vdir * m_vscale };
			svec<float, 3> vertex4{ current_pos +
				sc<float>(ch.left_pos) * m_hdir * m_hscale +
				sc<float>(ch.top_pos - ch.height) * m_vdir * m_vscale };
			float vertex_data[6][5] = {
				{ vertex1[0], vertex1[1], vertex1[2], 1.0f, 0.0f },
				{ vertex2[0], vertex2[1], vertex2[2], 1.0f, 1.0f },
				{ vertex3[0], vertex3[1], vertex3[2], 0.0f, 0.0f },
				{ vertex3[0], vertex3[1], vertex3[2], 0.0f, 0.0f },
				{ vertex2[0], vertex2[1], vertex2[2], 1.0f, 1.0f },
				{ vertex4[0], vertex4[1], vertex4[2], 0.0f, 1.0f },
			};
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo[i]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 5 * 6, vertex_data);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			offset += (ch.advance >> 6) * m_hscale;
		}
	}
}

#endif