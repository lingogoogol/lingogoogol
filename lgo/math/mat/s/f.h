#ifndef LGO_MAT_S_F_H
#define LGO_MAT_S_F_H

#include "../../vec/s/f.h"

namespace lgo {
	template<typename T1, int T2, int T3>
	class smat;

	template<typename T1>
	smat<T1, 4, 4> move(svec<T1, 3> pos);
	template<typename T1>
	smat<T1, 4, 4> rotate(svec<T1, 3> front, svec<T1, 3> up);
	template<typename T1>
	smat<float, 4, 4> ortho(float left, float right, float bottom, float top, float near, float far);
	template<typename T1>
	smat<T1, 4, 4> perspective(float angle, float aspect, float near, float far);
	template<typename T1, int T2, int T3>
	T1* make_gl_matrix(smat<T1, T2, T3> in);
	template<typename T1>
	void free_gl_matrix(T1* ptr);
}

#endif