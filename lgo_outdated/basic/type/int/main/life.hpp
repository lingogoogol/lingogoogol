#ifndef LGO_BASIC_TYPE_INT_MAIN_LIFE
#define LGO_BASIC_TYPE_INT_MAIN_LIFE

#include "def.hpp"

namespace lgo {
	template<is_basic_intp_cpt t_type>
	constexpr int_class<t_type>::int_class(): m_data{ 0 } {}
	
	template<is_basic_intp_cpt t_type>
	constexpr int_class<t_type>::int_class(type in): m_data{ in } {}
}

#endif