#ifndef LGO_TEXT_D_H
#define LGO_TEXT_D_H

#include <string>
#include <map>
#include <vector>

#include "f.h"
#include "../math/d.h"
#include "../vec/.h"
#include "../graphic/f.h"

namespace lgo {
	struct chartex {
		int advance{};
		int left_pos{};
		int top_pos{};
		int width{};
		int height{};
		unsigned int tex{};
		unsigned int count{};
	};

	class text {
	public:
		text(std::u32string text, float height, svec<float, 3> color_param,
			smat<float, 4, 4> transform_mat_param);
		void set_pos(svec<float, 3> pos_param);
		void set_horizontal_alignment(dir horizontal_alignment_param);
		void set_vertical_alignment(dir vertical_alignment_param);
		void set_height(float height);
		void set_horizontal_dir(svec<float, 3> horizontal_dir_param);
		void set_vertical_dir(svec<float, 3> vertical_dir_param);
		void set_text(std::u32string text_param);
		void render(unsigned int shader);
		svec<float, 3> m_color{};
		smat<float, 4, 4> m_transform_mat{};
	private:
		svec<float, 3> pos{ 0.0f,0.0f,0.0f };
		dir m_halign{ lgo::dir::middle };
		dir m_valign{ lgo::dir::middle };
		float m_hscale{};
		float m_vscale{};
		svec<float, 3> m_hdir{ 1.0f,0.0f,0.0f };
		svec<float, 3> m_vdir{ 0.0f,1.0f,0.0f };
		std::u32string m_text{};
		std::vector<unsigned int> m_vao{};
		std::vector<unsigned int> m_vbo{};
		void current_vbo();
	};
}

#endif