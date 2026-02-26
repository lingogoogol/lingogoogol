#ifndef LGO_MAT_S_I_H
#define LGO_MAT_S_I_H

#include "d.h"
#include "../../vec/s/i.h"

namespace lgo {
    template<typename... U1>
    smat(U1...)->smat<decltype(cm_type<U1...>{}[0]),
        sizeof...(U1), cm_type<U1...>{}.size()>;

    template<typename T1, int T2, int T3>
    smat<T1, T2, T3>::smat() :m_data{} {}

    template<typename T1, int T2, int T3>
    smat<T1, T2, T3>::smat(const T1* in) : smat{} {
        for (int row{ 0 }; row < rows(); row++)
            for (int col{ 0 }; col < cols(); col++)
                (*this)[row][col] = in[row * cols() + col];
    }

    template<typename T1, int T2, int T3>
    template<typename... U1>
    smat<T1, T2, T3>::smat(arrs<U1, T3>... in) : m_data{ in... } {}

    template<typename T1, int T2, int T3>
    arrs<T1, T3>& smat<T1, T2, T3>::operator[](int index) {
        return m_data[index];
    }

    template<typename T1, int T2, int T3>
    const arrs<T1, T3>& smat<T1, T2, T3>::operator[](int index) const {
        return m_data[index];
    }

    template<typename T1, int T2, int T3>
    template<int U1>
    smat<T1, T2, U1> smat<T1, T2, T3>::operator*(smat<T1, T3, U1> in) const {
        smat<T1, T2, U1> out{};
        for (int row{ 0 }; row < rows(); row++)
            for (int col{ 0 }; col < in.cols(); col++)
                for (int index{ 0 }; index < cols(); index++)
                    out[row][col] += (*this)[row][index] * in[index][col];
        return out;
    }

    template<typename T1, int T2, int T3>
    constexpr int smat<T1, T2, T3>::rows() const {
        return T2;
    }

    template<typename T1, int T2, int T3>
    constexpr int smat<T1, T2, T3>::cols() const {
        return T3;
    }

    template<typename T1, int T2, int T3>
    smat<T1, T2, T3>::operator svec<T1, T2>() const
        requires is_equal<T3, 1>&& is_lequal<T2, 4> {
        arrs<T1, T2> components{};
        for (int row{ 0 }; row < rows(); row++)
            components[row] = (*this)[row][0];
        return svec<T1, T2>{ components.data() };
    }

    template<typename T1, int T2, int T3>
    const T1* smat<T1, T2, T3>::data() const {
        return &m_data[0][0];
    }

    template<typename T1>
    smat<T1, 4, 4> move(svec<T1, 3> pos) {
        pos = -pos;
        return smat<T1, 4, 4>{
            arrs{ 1.0f, 0.0f, 0.0f, pos[0] },
                arrs{ 0.0f, 1.0f, 0.0f, pos[1] },
                arrs{ 0.0f, 0.0f, 1.0f, pos[2] },
                arrs{ 0.0f, 0.0f, 0.0f, 1.0f }
        };
    }

    template<typename T1>
    smat<T1, 4, 4> rotate(svec<T1, 3> front, svec<T1, 3> up) {
        svec<T1, 3> right{ normalize(cross(up, front)) };
        up = normalize(cross(front, right));
        front = normalize(front);
        return smat<T1, 4, 4>{
            arrs{ right[0], right[1], right[2], 0.0f },
                arrs{ up[0], up[1], up[2], 0.0f },
                arrs{ front[0], front[1], front[2], 0.0f },
                arrs{ 0.0f, 0.0f, 0.0f, 1.0f }
        };
    }

    template<typename T1>
    smat<T1, 4, 4> ortho(T1 left, T1 right, T1 bottom, T1 top, T1 near, T1 far) {
        return smat<T1, 4, 4>{
            arrs{ 2.0f / (right - left), 0.0f, 0.0f, -(right + left) / (right - left) },
                arrs{ 0.0f, 2.0f / (top - bottom), 0.0f, -(top + bottom) / (top - bottom) },
                arrs{ 0.0f, 0.0f, 2.0f / (far - near), -(far + near) / (far - near) },
                arrs{ 0.0f, 0.0f, 0.0f, 1.0f }
        };
    }

    template<typename T1>
    smat<T1, 4, 4> perspective(T1 angle, T1 aspect, T1 near, T1 far) {
        T1 tan{ std::tan(to_radians(angle) / 2) };
        return smat<T1, 4, 4>{
            arrs{ 1 / tan, 0.0f, 0.0f, 0.0f },
                arrs{ 0.0f, 1 / (tan * aspect), 0.0f, 0.0f },
                arrs{ 0.0f, 0.0f, (far + near) / (far - near), -(2.0f * near * far) / (far - near) },
                arrs{ 0.0f, 0.0f, 1.0f, 0.0f }
        };
    }

    template<typename T1, int T2, int T3>
    T1* make_gl_matrix(smat<T1, T2, T3> in) {
        T1* out{ new T1[in.cols() * in.rows()] };
        for (int col{ 0 }; col < in.cols(); col++)
            for (int row{ 0 }; row < in.rows(); row++)
                out[in.rows() * col + row] = in[row][col];
        return out;
    }

    template<typename T1>
    void free_gl_matrix(T1* ptr) {
        delete[] ptr;
    }
}

#endif