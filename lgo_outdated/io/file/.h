#ifndef LGO_FILE__H
#define LGO_FILE__H

#include <fstream>
#include <type_traits>

#include "../str/.h"
#include "../mem/.h"
#include "../type/fdmtl.h"

namespace lgo {
	template<is_char T1>
	class file;

	namespace impl {
		template<typename T1>
		struct is_file_stu;
		template<typename T1>
		struct is_file_stu<file<T1>>;
	}

	template<typename T1>
	concept is_file = impl::is_file_stu<T1>::val;

	template<is_char T1>
	class file {
	public:
		file(const strd& path);
		template<typename U1>
		file<T1>& push(const U1& src);
		template<non_dynamic U1>
		U1 get(intf64 pos);
		template<non_dynamic U1>
		U1 get(intf64 pos, intf64 len);
		template<typename U1>
		file<T1>& pad(const U1& src, intf64 pos);
		template<typename U1>
		file<T1>& pad(const U1& src, intf64 pos, intf64 len);
	private:
		std::fstream m_stream{};
		strd m_path{};
	};

	namespace impl {
		template<typename T1>
		struct is_file_stu {
			constexpr static bool val{ false };
		};

		template<typename T1>
		struct is_file_stu<file<T1>> {
			constexpr static bool val{ true };
		};
	}

	template<is_char T1>
	inline file<T1>::file(const strd& path) :m_stream{}, m_path{ path } {
		m_stream.open(m_path, std::ios::out | std::ios::app);
		m_stream.close();
	}

	template<is_char T1>
	template<typename U1>
	file<T1>& file<T1>::push(const U1& src) {//...
		m_stream.open(m_path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
		m_stream << src;
		m_stream.close();
		return *this;
	}

	template<is_char T1>
	template<non_dynamic U1>
	U1 file<T1>::get(intf64 pos) {
	}

	template<is_char T1>
	template<non_dynamic U1>
	U1 file<T1>::get(intf64 pos, intf64 len) {
	}

	template<is_char T1>
	template<typename U1>
	file<T1>& file<T1>::pad(const U1& src, intf64 pos) {
	}

	template<is_char T1>
	template<typename U1>
	file<T1>& file<T1>::pad(const U1& src, intf64 pos, intf64 len) {
	}
}

#endif