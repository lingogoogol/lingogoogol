#ifndef LGO_LINE_I_H
#define LGO_LINE_I_H

#include <cmath>
#include <limits>

#include "d.h"

namespace lgo {
    inline line::line(svec<float, 3> orig, svec<float, 3> point) :
        m_orig{ orig }, m_dir{ normalize(point - orig) } {}

    inline svec<float, 3> line::foot_of_perpendicular(svec<float, 3> point) {
        return m_orig + m_dir * dot(point - m_orig, m_dir) / dot(m_dir, m_dir);
    }

    inline float line::meet_plane(plane in) {
        float r_count{ dot(in.m_coeff, m_dir) };
        float constant{ in.m_const - dot(in.m_coeff, m_orig) };
        if (r_count == 0.0f) {
            if (constant == 0.0f)
                return std::numeric_limits<float>::infinity();
            else
                return std::numeric_limits<float>::quiet_NaN();
        }
        else
            return constant / r_count;
    }

    inline auto line::angle(line in) -> float {
        return std::acos(dot(m_dir, in.m_dir));
    }
}

#endif