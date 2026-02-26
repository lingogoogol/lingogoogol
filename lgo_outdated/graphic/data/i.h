#ifndef LGO_DATA_I_H
#define LGO_DATA_I_H

#include "d.h"

namespace lgo {
	namespace impl {
		inline data* get_data() {
			static data* instance{ new data{} };
			return instance;
		}
	}
}

#endif