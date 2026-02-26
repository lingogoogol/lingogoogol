#ifndef LGO_PLANE_I_H
#define LGO_PLANE_I_H

#include "d.h"

namespace lgo {
    inline plane::plane(svec<float, 3> point1,
        svec<float, 3> point2, svec<float, 3> point3) :
        m_coeff{ cross(point2 - point1, point3 - point1) }, m_const{} {
        m_const = dot(m_coeff, point1);
    }

    inline plane::plane(arrs<svec<float, 3>, 3> points) :
        plane{ points[0], points[1], points[2] } {}
}

#endif