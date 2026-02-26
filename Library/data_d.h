#ifndef LIBRARY_DATA_D_H
#define LIBRARY_DATA_D_H

#include "data.h"

namespace implement {
	inline Data* get_data() {
		static Data* data{};
		if (!get_data1())
			data = new Data{};
		return data;
	}

	inline bool get_data1() {
		static bool inited{ false };
		if (inited)
			return true;
		else {
			inited = true;
			return false;
		}
	}
}

#endif