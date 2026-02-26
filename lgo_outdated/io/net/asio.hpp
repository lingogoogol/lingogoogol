#ifndef LGO_IO_NET_ASIO
#define LGO_IO_NET_ASIO

namespace lgo {
	inline auto boost_asio() -> boost::asio::io_service& {
		static boost::asio::io_service asio{};
		return asio;
	}
}

#endif