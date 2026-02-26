#ifndef LGO_VEC_S_F_H
#define LGO_VEC_S_F_H

namespace lgo {
	template<typename T1, int T2>
	class svec;

	template<typename T1, int T2>
	svec<T1, T2> operator*(float in1, svec<T1, T2> in2);
	template<typename T1, int T2>
	T1 len(svec<T1, T2> in);
	template<typename T1, int T2>
	svec<T1, T2> normalize(svec<T1, T2> in);
	template<typename T1, int T2>
	T1 dot(svec<T1, T2> in1, svec<T1, T2> in2);
	template<typename T1, int T2>
	svec<T1, T2> cross(svec<T1, T2> in1, svec<T1, T2> in2);
}

#endif