#ifndef LGO_VEC_S_D_H
#define LGO_VEC_S_D_H

#include <string>

#include "f.h"
#include "../../arr/.h"
#include "../../mat/s/f.h"
#include "../../type/fdmtl.h"

namespace lgo {
	template<typename T1, int T2>
	class svec {
	public:
		svec();
		svec(const T1* in);
		template<typename... U1>
		svec(U1... in) requires is_same<rm_cvr<cm_type<U1...>>, T1>;
		svec(const svec<T1, T2>& in);
		T1& operator[](int index);
		const T1& operator[](int index) const;
		svec<T1, T2> operator-() const;
		svec<T1, T2> operator+(svec<T1, T2> in) const;
		svec<T1, T2> operator-(svec<T1, T2> in) const;
		svec<T1, T2> operator*(svec<T1, T2> in) const;
		svec<T1, T2> operator*(float in) const;
		template<typename U1, int U2>
		friend svec<U1, U2> operator*(float in1, const svec<U1, U2> in2);
		svec<T1, T2> operator/(svec<T1, T2> in) const;
		svec<T1, T2> operator/(float in) const;
		template<typename U1, int U2>
		friend svec<U1, U2> operator/(float in1, const svec<U1, U2> in2);
		auto operator=(const svec<T1, T2>& in) & -> svec&;
		svec<T1, T2>& operator+=(svec<T1, T2> in);
		svec<T1, T2>& operator-=(svec<T1, T2> in);
		svec<T1, T2>& operator*=(svec<T1, T2> in);
		svec<T1, T2>& operator*=(float in);
		svec<T1, T2>& operator/=(svec<T1, T2> in);
		svec<T1, T2>& operator/=(float in);
		bool operator==(svec<T1, T2> in) const;
		bool operator!=(svec<T1, T2> in) const;
		template<typename U1>
		operator svec<U1, T2>() const;
		operator smat<T1, T2, 1>() const;
		const T1* data() const;
		constexpr int size() const;
		T1 len() const;
		svec<T1, T2>& normalize();
		T1 dot(svec<T1, T2> in) const;
		svec<T1, T2>& cross(svec<T1, T2> in);
		std::string to_string8() const;
		std::u32string to_string32() const;
	private:
		arrs<T1, T2> m_data{};
	};
}

#endif