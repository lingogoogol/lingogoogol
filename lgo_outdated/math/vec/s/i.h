#ifndef LGO_VEC_S_I_H
#define LGO_VEC_S_I_H

#include "d.h"
#include "../../str/.h"
#include "../../mat/s/d.h"

namespace lgo {
    template<typename... U1>
    svec(U1...)->svec<cm_type<U1...>, sizeof...(U1)>;

    template<typename T1, int T2>
    svec<T1, T2>::svec() :m_data{} {}

    template<typename T1, int T2>
    svec<T1, T2>::svec(const T1* in) : svec{} {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] = in[i];
        }
    }

    template<typename T1, int T2>
    template<typename... U1>
    svec<T1, T2>::svec(U1... in) requires is_same<rm_cvr<cm_type<U1...>>, T1>
        : m_data{ in... } {}

    template<typename T1, int T2>
    svec<T1, T2>::svec(const svec<T1, T2>& in) : svec{} {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] = in[i];
        }
    }

    template<typename T1, int T2>
    T1& svec<T1, T2>::operator[](int index) {
        return m_data[index];
    }

    template<typename T1, int T2>
    const T1& svec<T1, T2>::operator[](int index) const {
        return m_data[index];
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator-() const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = -(*this)[i];
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator+(svec<T1, T2> in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] + in[i];
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator-(svec<T1, T2> in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] - in[i];
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator*(svec<T1, T2> in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] * in[i];
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator*(float in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] * in;
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator/(svec<T1, T2> in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] / in[i];
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2> svec<T1, T2>::operator/(float in) const {
        svec<T1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = (*this)[i] / in;
        }
        return out;
    }
    template<typename T1, int T2>
	auto svec<T1, T2>::operator=(const svec<T1, T2>& in) & -> svec& {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] = in[i];
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator+=(svec<T1, T2> in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] += in[i];
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator-=(svec<T1, T2> in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] -= in[i];
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator*=(svec<T1, T2> in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] *= in[i];
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator*=(float in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] *= in;
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator/=(svec<T1, T2> in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] /= in[i];
        }
        return *this;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::operator/=(float in) {
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] /= in;
        }
        return *this;
    }

    template<typename T1, int T2>
    bool svec<T1, T2>::operator==(svec<T1, T2> in) const {
        bool out{ true };
        for (int i{ 0 }; i < size(); ++i) {
            if ((*this)[i] != in[i]) {
                out = false;
            }
        }
        return out;
    }

    template<typename T1, int T2>
    bool svec<T1, T2>::operator!=(svec<T1, T2> in) const {
        return !(*this == in);
    }

    template<typename T1, int T2>
    template<typename U1>
    svec<T1, T2>::operator svec<U1, T2>() const {
        svec<U1, T2> out{};
        for (int i{ 0 }; i < size(); ++i) {
            out[i] = sc<U1>((*this)[i]);
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2>::operator smat<T1, T2, 1>() const {
        return smat<T1, T2, 1>{ data() };
    }

    template<typename T1, int T2>
    const T1* svec<T1, T2>::data() const {
        return &m_data[0];
    }

    template<typename T1, int T2>
    constexpr int svec<T1, T2>::size() const {
        return T2;
    }

    template<typename T1, int T2>
    T1 svec<T1, T2>::len() const {
        T1 len{ sc<T1>(0) };
        if constexpr (T2 == 4) {
            for (int i{ 0 }; i < 3; ++i) {
                len += std::pow((*this)[i], sc<T1>(2));
            }
            return std::pow(len, sc<T1>(0.5)) / (*this)[3];
        }
        else {
            for (int i{ 0 }; i < size(); ++i) {
                len += std::pow((*this)[i], sc<T1>(2));
            }
            return std::pow(len, sc<T1>(0.5));
        }
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::normalize() {
        T1 origlen{ len() };
        for (int i{ 0 }; i < size(); ++i) {
            (*this)[i] /= origlen;
        }
        return *this;
    }

    template<typename T1, int T2>
    T1 svec<T1, T2>::dot(svec<T1, T2> in) const {
        T1 out{ sc<T1>(0) };
        if constexpr (T2 == 4) {
            for (int i{ 0 }; i < 3; ++i) {
                out += (*this)[i] * in[i];
            }
        }
        else {
            for (int i{ 0 }; i < size(); ++i) {
                out += (*this)[i] * in[i];
            }
        }
        return out;
    }

    template<typename T1, int T2>
    svec<T1, T2>& svec<T1, T2>::cross(svec<T1, T2> in) {
        svec<T1, T2> out{};
        if constexpr (T2 <= 2) {
            throw U"";
        }
        else {
            for (int i{ 0 }; i < 3; ++i) {
                out[i] = (*this)[cycle_add(i, 0, 2)] * in[cycle_minus(i, 0, 2)] -
                    (*this)[cycle_minus(i, 0, 2)] * in[cycle_add(i, 0, 2)];
            }
        }
        *this = out;
        return *this;
    }

    template<typename T1, int T2>
    std::string svec<T1, T2>::to_string8() const {
        std::string out{ "(" };
        for (int i{ 0 }; i < size(); ++i) {
            out.insert(out.size(), lgo::to_string8<T1>((*this)[i], 2));
            if (i != size() - 1) {
                out.insert(out.size(), ", ");
            }
            else {
                out.push_back(')');
            }
        }
        return out;
    }

    template<typename T1, int T2>
    std::u32string svec<T1, T2>::to_string32() const {
        return lgo::to_string32(to_string8());
    }

    template<typename T1, int T2>
    svec<T1, T2> operator*(float in1, svec<T1, T2> in2) {
        return in2 * in1;
    }

    template<typename T1, int T2>
    T1 len(svec<T1, T2> in) {
        return in.len();
    }

    template<typename T1, int T2>
    svec<T1, T2> normalize(svec<T1, T2> in) {
        return in.normalize();
    }

    template<typename T1, int T2>
    T1 dot(svec<T1, T2> in1, svec<T1, T2> in2) {
        return in1.dot(in2);
    }

    template<typename T1, int T2>
    svec<T1, T2> cross(svec<T1, T2> in1, svec<T1, T2> in2) {
        return in1.cross(in2);
    }
}

#endif