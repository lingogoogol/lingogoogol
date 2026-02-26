#ifndef LGO_JSON_F_H
#define LGO_JSON_F_H

#include "../str/.h"

namespace lgo {
	enum class json_type;

	class json_pv;
	class json_num;
	class json_str;
	class json_bol;
	class json_arr;
	class json_obj;
	class json_nul;

	json_pv* make_json(const strd& in, intf64 begin = 0, intf64* end = nullptr);
}

#endif