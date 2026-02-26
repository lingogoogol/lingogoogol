#ifndef LGO_LINE_D_H
#define LGO_LINE_D_H

#include "f.h"
#include "../vec/.h"
#include "../plane/.h"

namespace lgo {
	class line {
	public:
		line(svec<float, 3> orig, svec<float, 3> point);
		svec<float, 3> foot_of_perpendicular(svec<float, 3> point);
		float meet_plane(plane in);
		auto angle(line in) -> float;
		svec<float, 3> m_orig{};
		svec<float, 3> m_dir{};
	};
}

#endif