#ifndef LGO_MAT_S_D_H
#define LGO_MAT_S_D_H

#include "f.h"
#include "../../arr/.h"

namespace lgo {
	template<typename T1, int T2, int T3>
	class smat {
	public:
		smat();
		smat(const T1* in);
		template<typename... U1>
		smat(arrs<U1, T3>... in);
		arrs<T1, T3>& operator[](int index);
		const arrs<T1, T3>& operator[](int index) const;
		template<int U1>
		smat<T1, T2, U1> operator*(smat<T1, T3, U1> in) const;
		operator svec<T1, T2>() const requires is_equal<T3, 1>&& is_lequal<T2, 4>;
		const T1* data() const;
		constexpr int rows() const;
		constexpr int cols() const;
	private:
		arrs<arrs<T1, T3>, T2> m_data{};
	};
}

#endif