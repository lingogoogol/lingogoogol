#ifndef LGO_PLANE_D_H
#define LGO_PLANE_D_H

#include "f.h"
#include "../arr/.h"
#include "../vec/.h"

namespace lgo {
	class plane {
	public:
		plane(svec<float, 3> point1, svec<float, 3> point2, svec<float, 3> point3);
		plane(arrs<svec<float, 3>, 3> points);
		svec<float, 3> m_coeff{};
		float m_const{};
	};
}

#endif